def PD_FeedOp : PD_Op<"feed", [NoSideEffect]> {
  let summary = "Feed Op";

  let description = [{
    Feed a tensor into the model.
  }];

  let arguments = (ins StrAttr:$name);
  let results = (outs PD_Tensor:$out);

  let assemblyFormat = [{
      `(` `)` attr-dict `:` type($out)
  }];
}

def PD_FetchOp : PD_Op<"fetch", [Terminator]> {
  let summary = "fetch Op";

  let description = [{
    Return the output tensor from the subgraph.
  }];

  let arguments = (ins PD_Tensor :$inputs, StrAttr:$name);
}

def PD_ConstantOp : PD_Op<"constant", [NoSideEffect, ConstantLike, DeclareOpInterfaceMethods<InferTypeOpInterface>, AllTypesMatch<["value", "output"]>]> {
  let summary = "constant Op";
  let description = [{}];

  let arguments = (ins ElementsAttr:$value);
  let results = (outs PD_Tensor:$output);
  let hasFolder = 1;

  let builders = [
    OpBuilder<(ins "mlir::Attribute":$value)>,
  ];
}
