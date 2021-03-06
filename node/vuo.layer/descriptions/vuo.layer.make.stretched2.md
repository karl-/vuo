Creates a layer that can be combined with other layers to create a composite image.  Unlike [Make Image Layer (Scaled)](vuo-node://vuo.layer.make.scaled), this node allows control over both the `Width` and `Height`.

The image used to create this layer may be resized in the composite image. If resizing makes the image larger, it may become blurry. If you want the image to be rendered at its original size, use the [Make Image Layer (Real Size)](vuo-node://vuo.layer.make.realSize2) node instead.

   - `Image` — The image displayed in the layer. For the layer to be visible, you have to provide an image.
   - `Anchor` — The point to transform the layer about.
   - `Transform` — A 2D transform that determines the translation, rotation, and scale of the layer relative to its parent.
   - `Opacity` — The image's opacity, from 0 (fully transparent) to 1 (fully opaque).
