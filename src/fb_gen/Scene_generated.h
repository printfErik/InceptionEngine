// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SCENE_INCEPTION_FB_H_
#define FLATBUFFERS_GENERATED_SCENE_INCEPTION_FB_H_

#include "flatbuffers/flatbuffers.h"

#include "component_generated.h"

namespace Inception {
namespace fb {

struct flatbufferEntity;
struct flatbufferEntityBuilder;

struct flatbufferTreeNode;
struct flatbufferTreeNodeBuilder;

struct flatbufferScene;
struct flatbufferSceneBuilder;

inline const flatbuffers::TypeTable *flatbufferEntityTypeTable();

inline const flatbuffers::TypeTable *flatbufferTreeNodeTypeTable();

inline const flatbuffers::TypeTable *flatbufferSceneTypeTable();

struct flatbufferEntity FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef flatbufferEntityBuilder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return flatbufferEntityTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_M_COMPONENTS_TYPE = 4,
    VT_M_COMPONENTS = 6
  };
  const flatbuffers::Vector<uint8_t> *m_components_type() const {
    return GetPointer<const flatbuffers::Vector<uint8_t> *>(VT_M_COMPONENTS_TYPE);
  }
  const flatbuffers::Vector<flatbuffers::Offset<void>> *m_components() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<void>> *>(VT_M_COMPONENTS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_M_COMPONENTS_TYPE) &&
           verifier.VerifyVector(m_components_type()) &&
           VerifyOffset(verifier, VT_M_COMPONENTS) &&
           verifier.VerifyVector(m_components()) &&
           VerifyicpComponentBaseVector(verifier, m_components(), m_components_type()) &&
           verifier.EndTable();
  }
};

struct flatbufferEntityBuilder {
  typedef flatbufferEntity Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_m_components_type(flatbuffers::Offset<flatbuffers::Vector<uint8_t>> m_components_type) {
    fbb_.AddOffset(flatbufferEntity::VT_M_COMPONENTS_TYPE, m_components_type);
  }
  void add_m_components(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<void>>> m_components) {
    fbb_.AddOffset(flatbufferEntity::VT_M_COMPONENTS, m_components);
  }
  explicit flatbufferEntityBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<flatbufferEntity> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<flatbufferEntity>(end);
    return o;
  }
};

inline flatbuffers::Offset<flatbufferEntity> CreateflatbufferEntity(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<uint8_t>> m_components_type = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<void>>> m_components = 0) {
  flatbufferEntityBuilder builder_(_fbb);
  builder_.add_m_components(m_components);
  builder_.add_m_components_type(m_components_type);
  return builder_.Finish();
}

inline flatbuffers::Offset<flatbufferEntity> CreateflatbufferEntityDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<uint8_t> *m_components_type = nullptr,
    const std::vector<flatbuffers::Offset<void>> *m_components = nullptr) {
  auto m_components_type__ = m_components_type ? _fbb.CreateVector<uint8_t>(*m_components_type) : 0;
  auto m_components__ = m_components ? _fbb.CreateVector<flatbuffers::Offset<void>>(*m_components) : 0;
  return Inception::fb::CreateflatbufferEntity(
      _fbb,
      m_components_type__,
      m_components__);
}

struct flatbufferTreeNode FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef flatbufferTreeNodeBuilder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return flatbufferTreeNodeTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_M_ENTITY = 4,
    VT_M_CHILDREN = 6
  };
  const Inception::fb::flatbufferEntity *m_entity() const {
    return GetPointer<const Inception::fb::flatbufferEntity *>(VT_M_ENTITY);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>> *m_children() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>> *>(VT_M_CHILDREN);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_M_ENTITY) &&
           verifier.VerifyTable(m_entity()) &&
           VerifyOffset(verifier, VT_M_CHILDREN) &&
           verifier.VerifyVector(m_children()) &&
           verifier.VerifyVectorOfTables(m_children()) &&
           verifier.EndTable();
  }
};

struct flatbufferTreeNodeBuilder {
  typedef flatbufferTreeNode Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_m_entity(flatbuffers::Offset<Inception::fb::flatbufferEntity> m_entity) {
    fbb_.AddOffset(flatbufferTreeNode::VT_M_ENTITY, m_entity);
  }
  void add_m_children(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>>> m_children) {
    fbb_.AddOffset(flatbufferTreeNode::VT_M_CHILDREN, m_children);
  }
  explicit flatbufferTreeNodeBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<flatbufferTreeNode> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<flatbufferTreeNode>(end);
    return o;
  }
};

inline flatbuffers::Offset<flatbufferTreeNode> CreateflatbufferTreeNode(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<Inception::fb::flatbufferEntity> m_entity = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>>> m_children = 0) {
  flatbufferTreeNodeBuilder builder_(_fbb);
  builder_.add_m_children(m_children);
  builder_.add_m_entity(m_entity);
  return builder_.Finish();
}

inline flatbuffers::Offset<flatbufferTreeNode> CreateflatbufferTreeNodeDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<Inception::fb::flatbufferEntity> m_entity = 0,
    const std::vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>> *m_children = nullptr) {
  auto m_children__ = m_children ? _fbb.CreateVector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>>(*m_children) : 0;
  return Inception::fb::CreateflatbufferTreeNode(
      _fbb,
      m_entity,
      m_children__);
}

struct flatbufferScene FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef flatbufferSceneBuilder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return flatbufferSceneTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_M_NAME = 4,
    VT_M_SCENEROOT = 6
  };
  const flatbuffers::String *m_name() const {
    return GetPointer<const flatbuffers::String *>(VT_M_NAME);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>> *m_sceneRoot() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>> *>(VT_M_SCENEROOT);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_M_NAME) &&
           verifier.VerifyString(m_name()) &&
           VerifyOffset(verifier, VT_M_SCENEROOT) &&
           verifier.VerifyVector(m_sceneRoot()) &&
           verifier.VerifyVectorOfTables(m_sceneRoot()) &&
           verifier.EndTable();
  }
};

struct flatbufferSceneBuilder {
  typedef flatbufferScene Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_m_name(flatbuffers::Offset<flatbuffers::String> m_name) {
    fbb_.AddOffset(flatbufferScene::VT_M_NAME, m_name);
  }
  void add_m_sceneRoot(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>>> m_sceneRoot) {
    fbb_.AddOffset(flatbufferScene::VT_M_SCENEROOT, m_sceneRoot);
  }
  explicit flatbufferSceneBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<flatbufferScene> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<flatbufferScene>(end);
    return o;
  }
};

inline flatbuffers::Offset<flatbufferScene> CreateflatbufferScene(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> m_name = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>>> m_sceneRoot = 0) {
  flatbufferSceneBuilder builder_(_fbb);
  builder_.add_m_sceneRoot(m_sceneRoot);
  builder_.add_m_name(m_name);
  return builder_.Finish();
}

inline flatbuffers::Offset<flatbufferScene> CreateflatbufferSceneDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *m_name = nullptr,
    const std::vector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>> *m_sceneRoot = nullptr) {
  auto m_name__ = m_name ? _fbb.CreateString(m_name) : 0;
  auto m_sceneRoot__ = m_sceneRoot ? _fbb.CreateVector<flatbuffers::Offset<Inception::fb::flatbufferTreeNode>>(*m_sceneRoot) : 0;
  return Inception::fb::CreateflatbufferScene(
      _fbb,
      m_name__,
      m_sceneRoot__);
}

inline const flatbuffers::TypeTable *flatbufferEntityTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_UTYPE, 1, 0 },
    { flatbuffers::ET_SEQUENCE, 1, 0 }
  };
  static const flatbuffers::TypeFunction type_refs[] = {
    Inception::fb::icpComponentBaseTypeTable
  };
  static const char * const names[] = {
    "m_components_type",
    "m_components"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 2, type_codes, type_refs, nullptr, nullptr, names
  };
  return &tt;
}

inline const flatbuffers::TypeTable *flatbufferTreeNodeTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_SEQUENCE, 0, 0 },
    { flatbuffers::ET_SEQUENCE, 1, 1 }
  };
  static const flatbuffers::TypeFunction type_refs[] = {
    Inception::fb::flatbufferEntityTypeTable,
    Inception::fb::flatbufferTreeNodeTypeTable
  };
  static const char * const names[] = {
    "m_entity",
    "m_children"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 2, type_codes, type_refs, nullptr, nullptr, names
  };
  return &tt;
}

inline const flatbuffers::TypeTable *flatbufferSceneTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_STRING, 0, -1 },
    { flatbuffers::ET_SEQUENCE, 1, 0 }
  };
  static const flatbuffers::TypeFunction type_refs[] = {
    Inception::fb::flatbufferTreeNodeTypeTable
  };
  static const char * const names[] = {
    "m_name",
    "m_sceneRoot"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 2, type_codes, type_refs, nullptr, nullptr, names
  };
  return &tt;
}

inline const Inception::fb::flatbufferScene *GetflatbufferScene(const void *buf) {
  return flatbuffers::GetRoot<Inception::fb::flatbufferScene>(buf);
}

inline const Inception::fb::flatbufferScene *GetSizePrefixedflatbufferScene(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<Inception::fb::flatbufferScene>(buf);
}

inline bool VerifyflatbufferSceneBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<Inception::fb::flatbufferScene>(nullptr);
}

inline bool VerifySizePrefixedflatbufferSceneBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<Inception::fb::flatbufferScene>(nullptr);
}

inline void FinishflatbufferSceneBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<Inception::fb::flatbufferScene> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedflatbufferSceneBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<Inception::fb::flatbufferScene> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace fb
}  // namespace Inception

#endif  // FLATBUFFERS_GENERATED_SCENE_INCEPTION_FB_H_
