/**
 * @file
 * TestVuoPoint2d implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoPoint2d.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint2d);

/**
 * Tests the VuoPoint2d type.
 */
class TestVuoPoint2d : public QObject
{
	Q_OBJECT

private slots:

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoPoint2d>("value");
		QTest::addColumn<bool>("testStringFromValue");

		{
			VuoPoint2d p;
			p.x = p.y = 0;
			QTest::newRow("zero") << "{\"x\":0,\"y\":0}" << p << true;
			QTest::newRow("emptystring") << "" << p << false;
			QTest::newRow("partial point 1") << "{\"y\":0}" << p << false;
			QTest::newRow("zero text") << QUOTE("0,0") << p << false;
			QTest::newRow("nonpoint") << "Otto von Bismarck" << p << false;
		}

		{
			VuoPoint2d p;
			p.x = p.y = 1;
			QTest::newRow("one") << "{\"x\":1,\"y\":1}" << p << true;
			QTest::newRow("one text") << QUOTE("1,1") << p << false;
		}

		{
			VuoPoint2d p;
			p.x = -0.5;
			p.y = 0.5;
			QTest::newRow("different values") << "{\"x\":-0.5,\"y\":0.5}" << p << true;
			QTest::newRow("different values array") << QUOTE([-.5,0.5]) << p << false;
			QTest::newRow("different values text") << QUOTE("-.5, .5") << p << false;
		}

		{
			VuoPoint2d p;
			p.x = 0;
			p.y = -0.999;
			QTest::newRow("partial point 2") << "{\"y\":-0.999}" << p << false;
		}

		{
			VuoPoint2d p;
			p.x = p.y = 3.40282e+38;
			QTest::newRow("float max") << "{\"x\":3.40282e+38, \"y\":3.40282e+38}" << p << false;
		}

		{
			VuoPoint2d p;
			p.x = p.y = 1.17549e-38;
			QTest::newRow("float min") << "{\"x\":1.17549e-38, \"y\":1.17549e-38}" << p << false;
		}
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoPoint2d, value);
		QFETCH(bool, testStringFromValue);

		VuoPoint2d p = VuoPoint2d_makeFromString(initializer.toUtf8().constData());

		QCOMPARE(p.x,value.x);
		QCOMPARE(p.y,value.y);

		if (testStringFromValue)
			QCOMPARE(VuoPoint2d_getString(p), initializer.toUtf8().constData());
	}
};

QTEST_APPLESS_MAIN(TestVuoPoint2d)

#include "TestVuoPoint2d.moc"

