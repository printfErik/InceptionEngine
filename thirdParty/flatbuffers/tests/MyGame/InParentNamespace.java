// automatically generated by the FlatBuffers compiler, do not modify

package MyGame;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class InParentNamespace extends Table {
  public static void ValidateVersion() { Constants.FLATBUFFERS_22_9_29(); }
  public static InParentNamespace getRootAsInParentNamespace(ByteBuffer _bb) { return getRootAsInParentNamespace(_bb, new InParentNamespace()); }
  public static InParentNamespace getRootAsInParentNamespace(ByteBuffer _bb, InParentNamespace obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __reset(_i, _bb); }
  public InParentNamespace __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }


  public static void startInParentNamespace(FlatBufferBuilder builder) { builder.startTable(0); }
  public static int endInParentNamespace(FlatBufferBuilder builder) {
    int o = builder.endTable();
    return o;
  }

  public static final class Vector extends BaseVector {
    public Vector __assign(int _vector, int _element_size, ByteBuffer _bb) { __reset(_vector, _element_size, _bb); return this; }

    public InParentNamespace get(int j) { return get(new InParentNamespace(), j); }
    public InParentNamespace get(InParentNamespace obj, int j) {  return obj.__assign(__indirect(__element(j), bb), bb); }
  }
  public InParentNamespaceT unpack() {
    InParentNamespaceT _o = new InParentNamespaceT();
    unpackTo(_o);
    return _o;
  }
  public void unpackTo(InParentNamespaceT _o) {
  }
  public static int pack(FlatBufferBuilder builder, InParentNamespaceT _o) {
    if (_o == null) return 0;
    startInParentNamespace(builder);
    return endInParentNamespace(builder);
  }
}
