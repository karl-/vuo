﻿/**
 * @file
 * VuoCompilerTriggerPort implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoPort.hh"
#include "VuoType.hh"

/**
 * Creates a trigger port based on @c portClass.
 */
VuoCompilerTriggerPort::VuoCompilerTriggerPort(VuoPort * basePort)
	: VuoCompilerPort(basePort)
{
}

/**
 * Generates code to create a heap-allocated PortContext, with the `triggerQueue` and `triggerSemaphore` initialized.
 *
 * @return A value of type `PortContext *`.
 */
Value * VuoCompilerTriggerPort::generateCreatePortContext(Module *module, BasicBlock *block)
{
	VuoType *dataType = getClass()->getDataVuoType();

	return VuoCompilerCodeGenUtilities::generateCreatePortContext(module, block,
																  dataType ? dataType->getCompiler()->getType() : NULL,
																  true, "org.vuo.composition." + getIdentifier());
}

/**
 * Generates code that schedules the worker function for this trigger to execute on the trigger's dispatch queue.
 *
 * The caller is responsible for filling in the body of @a workerFunction.
 *
 * If this trigger carries data, the argument to @a function is stored in a context value that is passed to @a workerFunction.
 */
void VuoCompilerTriggerPort::generateScheduleWorker(Module *module, Function *function, BasicBlock *block,
													Value *compositionStateValue, Value *eventIdValue,
													Value *portContextValue, VuoType *dataType,
													int minThreadsNeeded, int maxThreadsNeeded, int chainCount,
													Function *workerFunction)
{
	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	Value *dataCopyAsVoidPointer;
	if (dataType)
	{
		// PortDataType_retain(data);
		// PortDataType *dataCopy = (void *)malloc(sizeof(VuoImage));
		// *dataCopy = data;
		Value *dataValue = VuoCompilerCodeGenUtilities::unlowerArgument(dataType->getCompiler(), function, 0, module, block);
		dataType->getCompiler()->generateRetainCall(module, block, dataValue);
		ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));
		Value *dataCopyAddress = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, dataType->getCompiler()->getType(), oneValue);
		new StoreInst(dataValue, dataCopyAddress, false, block);
		dataCopyAsVoidPointer = new BitCastInst(dataCopyAddress, voidPointer, "", block);
	}
	else
	{
		dataCopyAsVoidPointer = ConstantPointerNull::get(voidPointer);
	}

	// unsigned long *eventIdCopy = (unsigned long *)malloc(sizeof(unsigned long));
	// *eventIdCopy = eventId;
	Type *eventIdType = VuoCompilerCodeGenUtilities::generateNoEventIdConstant(module)->getType();
	Value *eventIdCopyAddress = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, eventIdType, 1);
	new StoreInst(eventIdValue, eventIdCopyAddress, false, block);

	Value *contextValue = VuoCompilerCodeGenUtilities::generateCreateTriggerWorkerContext(module, block, compositionStateValue,
																						  dataCopyAsVoidPointer, eventIdCopyAddress);

	Value *dispatchQueueValue = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerQueue(module, block, portContextValue);

	VuoCompilerCodeGenUtilities::generateScheduleTriggerWorker(module, block, dispatchQueueValue, contextValue, workerFunction,
															   minThreadsNeeded, maxThreadsNeeded,
															   eventIdValue, compositionStateValue, chainCount);
}

/**
 * Generates code to submit a task to this trigger's dispatch queue. Returns the worker function, which will be called
 * by the dispatch queue to execute the task. The caller is responsible for filling in the body of the worker function.
 */
Function * VuoCompilerTriggerPort::generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *nodeContextValue,
																				string workerFunctionName, Value *workerFunctionArg)
{
	Function *workerFunction = getWorkerFunction(module, workerFunctionName);

	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Value *argAsVoidPointer;
	if (workerFunctionArg)
		argAsVoidPointer = new BitCastInst(workerFunctionArg, voidPointer, "", block);
	else
		argAsVoidPointer = ConstantPointerNull::get(voidPointer);

	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	Value *dispatchQueueValue = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerQueue(module, block, portContextValue);
	VuoCompilerCodeGenUtilities::generateSynchronousSubmissionToDispatchQueue(module, block, dispatchQueueValue, workerFunction, argAsVoidPointer);

	return workerFunction;
}

/**
 * Returns a function of type @c dispatch_function_t.
 */
Function * VuoCompilerTriggerPort::getWorkerFunction(Module *module, string functionName, bool isExternal)
{
	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<Type *> workerFunctionParams;
	workerFunctionParams.push_back(voidPointer);
	FunctionType *workerFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()),
														 workerFunctionParams,
														 false);

	Function *workerFunction = module->getFunction(functionName);
	if (! workerFunction) {
		workerFunction = Function::Create(workerFunctionType,
										  isExternal ? GlobalValue::ExternalLinkage : GlobalValue::InternalLinkage,
										  functionName,
										  module);
	}

	return workerFunction;
}

/**
 * Generates code to try to claim the trigger's semaphore. Returns the value returned by `dispatch_semaphore_wait`,
 * which can be checked to determine if the semaphore was claimed.
 */
Value * VuoCompilerTriggerPort::generateNonBlockingWaitForSemaphore(Module *module, BasicBlock *block, Value *portContextValue)
{
	ConstantInt *timeoutDeltaValue = ConstantInt::get(module->getContext(), APInt(64, 0));
	Value *timeoutValue = VuoCompilerCodeGenUtilities::generateCreateDispatchTime(module, block, timeoutDeltaValue);
	Value *dispatchSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerSemaphore(module, block, portContextValue);
	return VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, block, dispatchSemaphoreValue, timeoutValue);
}

/**
 * Generates code to signal the trigger's semaphore.
 */
void VuoCompilerTriggerPort::generateSignalForSemaphore(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	Value *dispatchSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerSemaphore(module, block, portContextValue);
	VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, block, dispatchSemaphoreValue);
}

/**
 * Generates code to get the function pointer for the trigger scheduler function.
 */
Value * VuoCompilerTriggerPort::generateLoadFunction(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	return VuoCompilerCodeGenUtilities::generateGetPortContextTriggerFunction(module, block, portContextValue, getClass()->getFunctionType());
}

/**
 * Generates code to set the function pointer for the trigger scheduler function.
 */
void VuoCompilerTriggerPort::generateStoreFunction(Module *module, BasicBlock *block, Value *nodeContextValue, Value *functionValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateSetPortContextTriggerFunction(module, block, portContextValue, functionValue);
}

/**
 * Generates code to get the data most recently fired from the trigger.
 */
Value * VuoCompilerTriggerPort::generateLoadPreviousData(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	return VuoCompilerCodeGenUtilities::generateGetPortContextData(module, block, portContextValue, getDataType());
}

/**
 * Generates code to deallocate the context and its contents created by generateAsynchronousSubmissionToDispatchQueue()
 * and passed to @a workerFunction.
 */
void VuoCompilerTriggerPort::generateFreeContext(Module *module, BasicBlock *block, Function *workerFunction)
{
	Value *contextValue = workerFunction->arg_begin();
	VuoCompilerCodeGenUtilities::generateFreeTriggerWorkerContext(module, block, contextValue);
}

/**
 * Generates code to get the composition state from the context created by generateAsynchronousSubmissionToDispatchQueue()
 * and passed to @a workerFunction.
 */
Value * VuoCompilerTriggerPort::generateCompositionStateValue(Module *module, BasicBlock *block, Function *workerFunction)
{
	// VuoCompositionState *compositionState = (VuoCompositionState *)((void **)context)[0];

	Value *contextValue = workerFunction->arg_begin();
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);
	Type *compositionStatePointerType = PointerType::get(VuoCompilerCodeGenUtilities::getCompositionStateType(module), 0);

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *compositionStateAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, (size_t)0);
	return new BitCastInst(compositionStateAsVoidPointer, compositionStatePointerType, "", block);
}

/**
 * Generates code to get the trigger data value from the context created by generateAsynchronousSubmissionToDispatchQueue()
 * and passed to @a workerFunction.
 */
Value * VuoCompilerTriggerPort::generateDataValue(Module *module, BasicBlock *block, Function *workerFunction)
{
	// PortDataType *dataCopy = (PortDataType *)((void **)context)[1]);
	// PortDataType data = *dataCopy;

	Value *contextValue = workerFunction->arg_begin();
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *dataCopyAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 1);
	PointerType *pointerToDataType = PointerType::get(getDataType(), 0);
	Value *dataCopyValue = new BitCastInst(dataCopyAsVoidPointer, pointerToDataType, "", block);
	return new LoadInst(dataCopyValue, "", block);
}

/**
 * Generates code to get the event ID from the context created by generateAsynchronousSubmissionToDispatchQueue()
 * and passed to @a workerFunction.
 */
Value * VuoCompilerTriggerPort::generateEventIdValue(Module *module, BasicBlock *block, Function *workerFunction)
{
	// unsigned long *eventIdCopy = (unsigned long *)((void **)context)[2];
	// unsigned long eventId = *eventIdCopy;

	Value *contextValue = workerFunction->arg_begin();
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);
	Type *eventIdType = VuoCompilerCodeGenUtilities::generateNoEventIdConstant(module)->getType();

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *eventIdCopyAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 2);
	PointerType *pointerToEventIdType = PointerType::get(eventIdType, 0);
	Value *eventIdCopyValue = new BitCastInst(eventIdCopyAsVoidPointer, pointerToEventIdType, "", block);
	return new LoadInst(eventIdCopyValue, "", block);
}

/**
 * Generates code to update the trigger's data value (if any) with the worker function argument.
 *
 * @return The trigger's new data value.
 */
Value * VuoCompilerTriggerPort::generateDataValueUpdate(Module *module, BasicBlock *block, Function *workerFunction, Value *nodeContextValue)
{
	Value *currentDataValue = NULL;
	Type *dataType = getDataType();

	if (dataType)
	{
		// Load the previous data.
		Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
		Value *previousDataValue = VuoCompilerCodeGenUtilities::generateGetPortContextData(module, block, portContextValue, dataType);

		// Load the current data.
		//
		// PortDataType *dataCopy = (PortDataType *)((void **)context)[1]);
		// PortDataType data = *dataCopy;
		currentDataValue = generateDataValue(module, block, workerFunction);

		// The current data becomes the previous data.
		//
		// PortDataType_release(previousData);
		// previousData = data;
		VuoCompilerCodeGenUtilities::generateSetPortContextData(module, block, portContextValue, currentDataValue);
		VuoCompilerType *type = getClass()->getDataVuoType()->getCompiler();
		type->generateReleaseCall(module, block, previousDataValue);
	}

	return currentDataValue;
}

/**
 * Generates code to discard the scheduler function argument without updating the trigger's data value.
 */
void VuoCompilerTriggerPort::generateDataValueDiscardFromScheduler(Module *module, Function *function, BasicBlock *block,
																   VuoType *dataType)
{
	Value *dataValue = VuoCompilerCodeGenUtilities::unlowerArgument(dataType->getCompiler(), function, 0, module, block);
	dataType->getCompiler()->generateRetainCall(module, block, dataValue);
	dataType->getCompiler()->generateReleaseCall(module, block, dataValue);
}

/**
 * Generates code to discard the data value in the worker function argument without updating the trigger's data value.
 */
void VuoCompilerTriggerPort::generateDataValueDiscardFromWorker(Module *module, BasicBlock *block, Function *workerFunction)
{
	VuoType *dataType = getClass()->getDataVuoType();

	if (dataType)
	{
		Value *dataValue = generateDataValue(module, block, workerFunction);
		dataType->getCompiler()->generateReleaseCall(module, block, dataValue);
	}
}

/**
 * Returns the type of data associated with the trigger, or NULL if the trigger is event-only.
 */
Type * VuoCompilerTriggerPort::getDataType(void)
{
	VuoType *vuoType = getClass()->getDataVuoType();
	return (vuoType == NULL ? NULL : vuoType->getCompiler()->getType());
}

/**
 * Returns the trigger port class of this trigger port.
 */
VuoCompilerTriggerPortClass * VuoCompilerTriggerPort::getClass(void)
{
	return (VuoCompilerTriggerPortClass *)(getBase()->getClass()->getCompiler());
}
