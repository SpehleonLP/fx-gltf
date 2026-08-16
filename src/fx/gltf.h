// ------------------------------------------------------------
// Copyright(c) 2018 Jesse Yurkovich
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// See the LICENSE file in the repo root for full license information.
// ------------------------------------------------------------
#pragma once

#include "expected/include/tl/expected.hpp"
#include <array>
#include <cstring>
#include <optional>
#include <span>
#include <unordered_map>
#include <string>
#include <vector>
#include <cmath>
#include <filesystem>

#include <nlohmann/json.hpp>
#define GLTF_SAVE 1

#if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17 == 1)
#define FX_GLTF_HAS_CPP_17
#include <string_view>
#endif

#ifndef JSON_ERROR
#define JSON_ERROR
struct JsonError
{
	std::string file;
	std::string what;
};
#endif

namespace fx
{
namespace base64
{
    namespace detail
    {
        // clang-format off
        constexpr std::array<char, 64> EncodeMap =
        {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
            'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
        };

        constexpr std::array<char, 256> DecodeMap =
        {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
            -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
            -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        };
        // clang-format on
    } // namespace detail

} // namespace base64

namespace gltf
{
    namespace detail
    {
        struct ChunkHeader
        {
            uint32_t chunkLength{};
            uint32_t chunkType{};
        };

        struct GLBHeader
        {
            uint32_t magic{};
            uint32_t version{};
            uint32_t length{};

            ChunkHeader jsonHeader{};
        };

        constexpr uint32_t DefaultMaxBufferCount = 8;
        constexpr uint32_t DefaultMaxMemoryAllocation = 1024 * 1024 * 1024;
        constexpr std::size_t HeaderSize{ sizeof(GLBHeader) };
        constexpr std::size_t ChunkHeaderSize{ sizeof(ChunkHeader) };
        constexpr uint32_t GLBHeaderMagic = 0x46546c67u;
        // Our lf_glb CBOR variant. Same GLB header + BIN framing as a standard GLB;
        // only the file magic differs, so the loader distinguishes the two by the
        // first 4 bytes and decodes the structural chunk as CBOR instead of JSON.
        // Little-endian target: this u32 equals the bytes 'C','B','O','R' in file order.
        constexpr uint32_t GLBHeaderMagicCBOR = 0x524f4243u;
        constexpr uint32_t GLBChunkJSON = 0x4e4f534au;
        constexpr uint32_t GLBChunkBIN = 0x004e4942u;

        constexpr char const * const MimetypeApplicationOctet = "data:application/octet-stream;base64";
        constexpr char const * const MimetypeImagePNG = "data:image/png;base64";
        constexpr char const * const MimetypeImageJPG = "data:image/jpeg;base64";
	}

	class invalid_gltf_document : public std::runtime_error
    {
    public:
        explicit invalid_gltf_document(char const * message)
            : std::runtime_error(message)
        {
        }

        invalid_gltf_document(char const * message, std::string_view const& extra)
            : std::runtime_error(CreateMessage(message, extra).c_str())
        {
        }

    private:
        std::string CreateMessage(char const * message, std::string_view const& extra)
        {
            return std::string(message).append(" : ").append(extra);
        }
    };

	struct Buffer;
	struct DataContext;
	struct Document;

	namespace detail
	{
#if defined(FX_GLTF_HAS_CPP_17)
        template <typename TTarget>
        inline void ReadRequiredField(std::string_view key, nlohmann::json const& json, TTarget & target)
#else
        template <typename TKey, typename TTarget>
        inline void ReadRequiredField(TKey && key, nlohmann::json const& json, TTarget & target)
#endif
        {
            const nlohmann::json::const_iterator iter = json.find(key);
            if (iter == json.end())
            {
                throw invalid_gltf_document("Required field not found", std::string(key));
            }

            target = iter->get<TTarget>();
        }

#if defined(FX_GLTF_HAS_CPP_17)
        template <typename TTarget>
        inline bool ReadOptionalField(std::string_view key, nlohmann::json const& json, TTarget & target)
#else
        template <typename TKey, typename TTarget>
        inline bool ReadOptionalField(TKey && key, nlohmann::json const& json, TTarget & target)
#endif
        {
            const nlohmann::json::const_iterator iter = json.find(key);
            if (iter != json.end())
            {
                target = iter->get<TTarget>();
				return true;
            }

			return false;
        }

		template <typename TValue>
        inline void WriteField(std::string const& key, nlohmann::json & json, TValue const& value)
        {
            if (!value.empty())
            {
                json[key] = value;
            }
        }

		template <typename TValue>
        inline void WriteRequiredField(std::string const& key, nlohmann::json & json, TValue const& value)
        {
             json[key] = value;
        }


        template <typename TValue>
        inline void WriteField(std::string const& key, nlohmann::json & json, TValue const& value, TValue const& defaultValue)
        {
            if (value != defaultValue)
            {
                json[key] = value;
            }
        }


		void ReadExtensionsAndExtras(nlohmann::json const& json, nlohmann::json & extensionsAndExtras);
		void WriteExtensions(nlohmann::json & json, nlohmann::json const& extensionsAndExtras);

		std::size_t GetFileSize(std::istream & file);
		std::string GetDocumentRootPath(std::string_view const& documentFilePath);
		std::string CreateBufferUriPath(std::string_view const& documentRootPath, std::string_view const& bufferUri);

		void LoadBuffers(std::vector<Buffer> & buffers, DataContext const& dataContext);
		void ValidateBuffers(std::vector<Buffer> const& buffers, bool useBinaryFormat);
		[[nodiscard]] std::optional<JsonError> Save(nlohmann::json && json, std::vector<Buffer> const& buffers, std::string const& documentFilePath, bool useBinaryFormat, uint32_t Magic);
    } // namespace detail

    namespace defaults
    {
        constexpr std::array<float, 16> IdentityMatrix{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        constexpr std::array<float, 4> IdentityRotation{ 0, 0, 0, 1 };
        constexpr std::array<float, 4> IdentityVec4{ 1, 1, 1, 1 };
        constexpr std::array<float, 3> IdentityVec3{ 1, 1, 1 };
        constexpr std::array<float, 3> NullVec3{ 0, 0, 0 };
        constexpr float IdentityScalar = 1;
        constexpr float FloatSentinel = 10000;

        constexpr bool AccessorNormalized = false;

        constexpr float MaterialAlphaCutoff = 0.5f;
        constexpr bool MaterialDoubleSided = false;
    } // namespace defaults

    using Attributes = std::unordered_map<std::string, uint32_t>;

    struct NeverEmpty
    {
        bool empty() const noexcept
        {
            return false;
        }
    };
	
	struct AccessorBlueprint
    {
		enum class ComponentType : uint16_t
        {
            None = 0,
            Byte = 5120,
            UnsignedByte = 5121,
            Short = 5122,
            UnsignedShort = 5123,
			Int = 5124,
            UnsignedInt = 5125,
            Float = 5126
        };

        enum class Type : uint8_t
        {
            None,
            Scalar,
            Vec2,
            Vec3,
            Vec4,
            Mat2,
            Mat3,
            Mat4
        };

		static int32_t GetComponentSizeInBytes(ComponentType componentType)
		{
			switch(componentType)
			{
			default:							return 0;
			case ComponentType::Byte:			return 1;
			case ComponentType::UnsignedByte:	return 1;
			case ComponentType::Short:			return 2;
			case ComponentType::UnsignedShort:	return 2;
			case ComponentType::Int:			return 4;
			case ComponentType::UnsignedInt:	return 4;
			case ComponentType::Float:			return 4;
			}
		}
		static int32_t GetNoComponentsInType(Type type)
		{
			switch(type)
			{
			default:			return 0;
			case Type::Scalar:	return 1;
			case Type::Vec2:	return 2;
			case Type::Vec3:	return 3;
			case Type::Vec4:	return 4;
			case Type::Mat2:	return 4;
			case Type::Mat3:	return 9;
			case Type::Mat4:	return 16;
			}
		}
		static int32_t GetTypeSizeInBytes(Type type, ComponentType componentType)  { return GetNoComponentsInType(type) * GetComponentSizeInBytes(componentType); }

		inline int32_t GetComponentSizeInBytes() const { return GetComponentSizeInBytes(componentType); }
		inline int32_t GetNoComponentsInType() const { return GetNoComponentsInType(type); }
		inline int32_t GetTypeSizeInBytes() const { return GetTypeSizeInBytes(type, componentType); }
		inline uint32_t byteLength() const { return GetTypeSizeInBytes(type, componentType) * count; }

        int32_t bufferView{ -1 };
        uint32_t byteOffset{};
        uint32_t count{};
        bool normalized{ defaults::AccessorNormalized };

        ComponentType componentType{ ComponentType::None };
        Type type{ Type::None };
	};
	
    struct Accessor : public AccessorBlueprint
    {
	typedef AccessorBlueprint::ComponentType ComponentType;
	typedef AccessorBlueprint::Type Type;
		
        struct Sparse
        {
            struct Indices : NeverEmpty
            {
                uint32_t bufferView{};
                uint32_t byteOffset{};
                ComponentType componentType{ ComponentType::None };

                nlohmann::json extensionsAndExtras{};
            };

            struct Values : NeverEmpty
            {
                uint32_t bufferView{};
                uint32_t byteOffset{};

                nlohmann::json extensionsAndExtras{};
            };

            int32_t count{};
            Indices indices{};
            Values values{};

            nlohmann::json extensionsAndExtras{};

            bool empty() const noexcept
            {
                return count == 0;
            }
        };

        Sparse sparse{};

        std::string name;
        std::vector<float> max{};
        std::vector<float> min{};

        nlohmann::json extensionsAndExtras{};
    };

    struct Animation
    {
        struct Channel
        {
            struct Target : NeverEmpty
            {
				enum class Path
				{
					Undefined = -1,
					Translation,
					Rotation,
					Scale,
					Weights
				};

                int32_t node{ -1 };
                Path    path{Target::Path::Translation};

                nlohmann::json extensionsAndExtras{};
            };

            int32_t sampler{ -1 };
            Target target{};

            nlohmann::json extensionsAndExtras{};

        };

        struct Sampler
        {
            enum class Type
            {
                Linear,
                Step,
                CubicSpline
            };

            int32_t input{ -1 };
            int32_t output{ -1 };

            Type interpolation{ Sampler::Type::Linear };

            nlohmann::json extensionsAndExtras{};
        };

        std::string name{};
        std::vector<Channel> channels{};
        std::vector<Sampler> samplers{};

        nlohmann::json extensionsAndExtras{};
    };

    struct Asset : NeverEmpty
    {
        std::string copyright{};
        std::string generator{};
        std::string minVersion{};
        std::string version{ "2.0" };

        nlohmann::json extensionsAndExtras{};
    };

    struct Buffer
    {
        uint32_t byteLength{};

        std::string name;
        std::string uri;

        nlohmann::json extensionsAndExtras{};

        std::vector<uint8_t> data{};

        bool IsEmbeddedResource() const noexcept;
        void SetEmbeddedResource();
    };

    struct BufferView
    {
        enum class TargetType : uint16_t
        {
            None = 0,
            ArrayBuffer = 34962,
            ElementArrayBuffer = 34963
        };

        std::string name;

        int32_t buffer{ -1 };
        uint32_t byteOffset{};
        uint32_t byteLength{};
        uint32_t byteStride{};

        TargetType target{ TargetType::None };

        nlohmann::json extensionsAndExtras{};
    };

    struct Camera
    {
        enum class Type
        {
            None,
            Orthographic,
            Perspective
        };

        struct Orthographic : NeverEmpty
        {
            float xmag{ defaults::FloatSentinel };
            float ymag{ defaults::FloatSentinel };
            float zfar{ -defaults::FloatSentinel };
            float znear{ -defaults::FloatSentinel };

            nlohmann::json extensionsAndExtras{};
        };

        struct Perspective : NeverEmpty
        {
            float aspectRatio{};
            float yfov{};
            float zfar{std::nanf("")};
            float znear{};

            nlohmann::json extensionsAndExtras{};
        };

        std::string name{};
        Type type{ Type::None };

        Orthographic orthographic;
        Perspective perspective;

        nlohmann::json extensionsAndExtras{};
    };

    struct Image
    {
        int32_t bufferView{-1};

        std::string name;
        std::string uri;
        std::string mimeType;

        nlohmann::json extensionsAndExtras{};

        bool IsEmbeddedResource() const noexcept;
        void MaterializeData(std::vector<uint8_t> & data) const;
    };

    struct Material
    {
        enum class AlphaMode : uint8_t
        {
            Opaque,
            Mask,
            Blend
        };

        struct Texture
        {
            int32_t index{ -1 };
            int32_t texCoord{};

            nlohmann::json extensionsAndExtras{};

            bool empty() const noexcept
            {
                return index == -1;
            }
			
			bool operator==(Texture const& it) const { 
				return index == it.index 
					&& texCoord == it.texCoord 
					&& extensionsAndExtras == it.extensionsAndExtras;
			}
        };

        struct NormalTexture : Texture
        {
            float scale{ defaults::IdentityScalar };
        };

        struct OcclusionTexture : Texture
        {
            float strength{ defaults::IdentityScalar };
        };

        struct PBRMetallicRoughness
        {
            std::array<float, 4> baseColorFactor = { defaults::IdentityVec4 };
            Texture baseColorTexture;

            float roughnessFactor{ defaults::IdentityScalar };
            float metallicFactor{ defaults::IdentityScalar };
            Texture metallicRoughnessTexture;

            nlohmann::json extensionsAndExtras{};

            bool empty() const
            {
                return baseColorTexture.empty() && metallicRoughnessTexture.empty() && metallicFactor == 1.0f && roughnessFactor == 1.0f && baseColorFactor == defaults::IdentityVec4;
            }
        };

        float alphaCutoff{ defaults::MaterialAlphaCutoff };
        AlphaMode alphaMode{ AlphaMode::Opaque };

        bool doubleSided{ defaults::MaterialDoubleSided };

        NormalTexture normalTexture;
        OcclusionTexture occlusionTexture;
        PBRMetallicRoughness pbrMetallicRoughness;

        Texture emissiveTexture;
        std::array<float, 3> emissiveFactor = { defaults::NullVec3 };

        std::string name;
        nlohmann::json extensionsAndExtras{};
    };

    struct Primitive
    {
        enum class Mode : uint8_t
        {
            Points = 0,
            Lines = 1,
            LineLoop = 2,
            LineStrip = 3,
            Triangles = 4,
            TriangleStrip = 5,
            TriangleFan = 6
        };

        int32_t indices{ -1 };
        int32_t material{ -1 };

        Mode mode{ Mode::Triangles };

        Attributes attributes{};
        std::vector<Attributes> targets{};

        nlohmann::json extensionsAndExtras{};
    };

    struct Mesh
    {
        std::string name;

        std::vector<float> weights{};
        std::vector<Primitive> primitives{};

        nlohmann::json extensionsAndExtras{};
    };

    struct Node
    {
        std::string name;

        int32_t camera{ -1 };
        int32_t mesh{ -1 };
        int32_t skin{ -1 };

        std::array<float, 16> matrix{ defaults::IdentityMatrix };
        std::array<float, 4> rotation{ defaults::IdentityRotation };
        std::array<float, 3> scale{ defaults::IdentityVec3 };
        std::array<float, 3> translation{ defaults::NullVec3 };

        std::vector<int32_t> children{};
        std::vector<float> weights{};

        nlohmann::json extensionsAndExtras{};
    };

    struct Sampler
    {
        enum class MagFilter : uint16_t
        {
            None,
            Nearest = 9728,
            Linear = 9729
        };

        enum class MinFilter : uint16_t
        {
            None,
            Nearest = 9728,
            Linear = 9729,
            NearestMipMapNearest = 9984,
            LinearMipMapNearest = 9985,
            NearestMipMapLinear = 9986,
            LinearMipMapLinear = 9987
        };

        enum class WrappingMode : uint16_t
        {
            ClampToEdge = 33071,
            MirroredRepeat = 33648,
            Repeat = 10497
        };

        std::string name;

        MagFilter magFilter{ MagFilter::None };
        MinFilter minFilter{ MinFilter::None };

        WrappingMode wrapS{ WrappingMode::Repeat };
        WrappingMode wrapT{ WrappingMode::Repeat };

        nlohmann::json extensionsAndExtras{};

        bool empty() const noexcept
        {
            return name.empty() && magFilter == MagFilter::None && minFilter == MinFilter::None && wrapS == WrappingMode::Repeat && wrapT == WrappingMode::Repeat && extensionsAndExtras.empty();
        }
    };

    struct Scene
    {
        std::string name;

        std::vector<uint32_t> nodes{};

        nlohmann::json extensionsAndExtras{};
    };

    struct Skin
    {
        int32_t inverseBindMatrices{ -1 };
        int32_t skeleton{ -1 };

        std::string name;
        std::vector<uint32_t> joints{};

        nlohmann::json	extensionsAndExtras{};
    };

    struct Texture
    {
        std::string name;

        int32_t sampler{ -1 };
        int32_t source{ -1 };

#ifdef 	CHEETAH
		int32_t texCoords{-1};
#endif

        nlohmann::json extensionsAndExtras{};
    };

	struct DocumentBase
	{
		std::string name;
		std::vector<Texture> textures{};
		std::vector<Sampler> samplers{};
		std::vector<Image>   images{};

		std::vector<Accessor> accessors{};
		std::vector<BufferView> bufferViews{};
		std::vector<Buffer> buffers{};
	};

    struct Document : public DocumentBase
    {
        Asset asset;

        std::vector<Animation> animations{};
        std::vector<Camera> cameras{};
        std::vector<Material> materials{};
        std::vector<Mesh> meshes{};
        std::vector<Node> nodes{};
        std::vector<Scene> scenes{};
        std::vector<Skin> skins{};

        int32_t scene{ -1 };
        std::vector<std::string> extensionsUsed{};
        std::vector<std::string> extensionsRequired{};

        nlohmann::json extensionsAndExtras{};
    };

    struct ReadQuotas
    {
        uint32_t MaxBufferCount{ detail::DefaultMaxBufferCount };
        uint32_t MaxFileSize{ detail::DefaultMaxMemoryAllocation };
        uint32_t MaxBufferByteLength{ detail::DefaultMaxMemoryAllocation };
    };

	struct DataContext
	{
		std::string bufferRootPath{};
		ReadQuotas readQuotas;

		std::span<uint8_t> binaryData{};
		std::size_t binaryOffset{};
	};
	
	tl::expected<Document, JsonError> Load(std::filesystem::path const& documentFilePath);	
	tl::expected<Document, JsonError> LoadFromText(std::istream & input, const std::string &documentRootPath, ReadQuotas const & readQuotas = {});
    tl::expected<Document, JsonError> LoadFromText(std::string const& documentFilePath, bool skip_buffers = false, ReadQuotas const& readQuotas = {});
	
	tl::expected<Document, JsonError> LoadFromBinary(std::istream & input, std::string const & documentRootPath, ReadQuotas const & readQuotas = {});
    tl::expected<Document, JsonError> LoadFromBinary(std::string const& documentFilePath, bool skip_buffers = false, ReadQuotas const& readQuotas = {});
	tl::expected<Document, JsonError> LoadFromBinary(std::vector<uint8_t> binary, std::string const& documentFilePath, bool skip_buffers = false, ReadQuotas const& readQuotas = {});

	[[nodiscard]] std::optional<JsonError> Save(Document const & document, std::ostream & output, const std::string &documentRootPath, bool useBinaryFormat, bool useCbor = false);
    [[nodiscard]] std::optional<JsonError> Save(Document const& document, std::string documentFilePath, bool useBinaryFormat);	
	

	[[nodiscard]] std::optional<JsonError> LoadExternalBuffers(Document & document, std::string const& documentFilePath, ReadQuotas const& readQuotas = {});
	
	
	void from_json(nlohmann::json const& json, Image & buffer);
	void from_json(nlohmann::json const& json, BufferView & buffer);
	void from_json(nlohmann::json const& json, Buffer & buffer);

	void to_json(nlohmann::json & json, Image const& buffer);
	void to_json(nlohmann::json & json, Buffer const& buffer);
	void to_json(nlohmann::json & json, BufferView const& buffer);
	void to_json(nlohmann::json & json, Document const& document);

} // namespace gltf


} // namespace fx

#undef FX_GLTF_HAS_CPP_17
