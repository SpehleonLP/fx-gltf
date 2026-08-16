#pragma GCC diagnostic ignored "-Wredundant-move"
#include "gltf.h"
// ------------------------------------------------------------
// Copyright(c) 2018 Jesse Yurkovich
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// See the LICENSE file in the repo root for full license information.
// ------------------------------------------------------------

#include <array>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#define GLTF_SAVE 1

#if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17 == 1)
#define FX_GLTF_HAS_CPP_17
#include <string_view>
#endif


static std::string decodeURIPercentEncoding(const std::string& encoded) {
	std::string decoded;
	std::istringstream iss(encoded);
	char ch;

	while (iss >> std::noskipws >> ch) {
		if (ch == '%') {
			std::string hex;
			hex.push_back(iss.get());
			hex.push_back(iss.get());
			std::istringstream hexStream(hex);
			int value;
			hexStream >> std::hex >> value;
			decoded += static_cast<char>(value);
		} else {
			decoded += ch;
		}
	}

	return decoded;
}


namespace fx
{
namespace base64
{
    namespace detail
    {
        // clang-format on
    } // namespace detail

     std::string Encode(std::vector<uint8_t> const& bytes)
    {
        const std::size_t length = bytes.size();
        if (length == 0)
        {
            return {};
        }

        std::string out{};
        out.reserve(((length * 4 / 3) + 3) & (~3u)); // round up to nearest 4

        uint32_t value = 0;
        int32_t bitCount = -6;
        for (const uint8_t c : bytes)
        {
            value = (value << 8u) + c;
            bitCount += 8;
            while (bitCount >= 0)
            {
                const uint32_t shiftOperand = bitCount;
                out.push_back(detail::EncodeMap.at((value >> shiftOperand) & 0x3fu));
                bitCount -= 6;
            }
        }

        if (bitCount > -6)
        {
            const uint32_t shiftOperand = bitCount + 8;
            out.push_back(detail::EncodeMap.at(((value << 8u) >> shiftOperand) & 0x3fu));
        }

        while (out.size() % 4 != 0)
        {
            out.push_back('=');
        }

        return out;
    }

#if defined(FX_GLTF_HAS_CPP_17)
     bool TryDecode(std::string_view in, std::vector<uint8_t> & out)
#else
     bool TryDecode(std::string const& in, std::vector<uint8_t> & out)
#endif
    {
        out.clear();

        const std::size_t length = in.length();
        if (length == 0)
        {
            return true;
        }

        if (length % 4 != 0)
        {
            return false;
        }

        out.reserve((length / 4) * 3);

        bool invalid = false;
        uint32_t value = 0;
        int32_t bitCount = -8;
        for (std::size_t i = 0; i < length; i++)
        {
            const uint8_t c = static_cast<uint8_t>(in[i]);
            const char map = detail::DecodeMap.at(c);
            if (map == -1)
            {
                if (c != '=') // Non base64 character
                {
                    invalid = true;
                }
                else
                {
                    // Padding characters not where they should be
                    const std::size_t remaining = length - i - 1;
                    if (remaining > 1 || (remaining == 1 ? in[i + 1] != '=' : false))
                    {
                        invalid = true;
                    }
                }

                break;
            }

            value = (value << 6u) + map;
            bitCount += 6;
            if (bitCount >= 0)
            {
                const uint32_t shiftOperand = bitCount;
                out.push_back(static_cast<uint8_t>(value >> shiftOperand));
                bitCount -= 8;
            }
        }

        if (invalid)
        {
            out.clear();
        }

        return !invalid;
    }
} // namespace base64

namespace gltf
{


    namespace detail
    {


        void ReadExtensionsAndExtras(nlohmann::json * json, nlohmann::json & extensionsAndExtras)
        {
            nlohmann::json::iterator iterExtensions = json->find("extensions");
            nlohmann::json::iterator iterExtras = json->find("extras");
            if (iterExtensions != json->end())
            {
                extensionsAndExtras["extensions"] = std::move(*iterExtensions);
            }

            if (iterExtras != json->end())
            {
                extensionsAndExtras["extras"] = std::move(*iterExtras);
            }
        }

        void ReadExtensionsAndExtras(nlohmann::json const& json, nlohmann::json & extensionsAndExtras)
        {
            nlohmann::json::const_iterator iterExtensions = json.find("extensions");
            nlohmann::json::const_iterator iterExtras = json.find("extras");
            if (iterExtensions != json.end())
            {
                extensionsAndExtras["extensions"] = (*iterExtensions);
            }

            if (iterExtras != json.end())
            {
                extensionsAndExtras["extras"] = (*iterExtras);
            }
        }

        void WriteExtensions(nlohmann::json & json, nlohmann::json const& extensionsAndExtras)
        {
            if (!extensionsAndExtras.empty())
            {
                for (nlohmann::json::const_iterator it = extensionsAndExtras.begin(); it != extensionsAndExtras.end(); ++it)
                {
                    json[it.key()] = it.value();
                }
            }
        }

        std::string GetDocumentRootPath(std::string_view const& documentFilePath)
        {
            const std::size_t pos = documentFilePath.find_last_of("/\\");
            if (pos != std::string::npos)
            {
                return std::string(documentFilePath.substr(0, pos));
            }

            return {};
        }
		
		std::string CreateBufferUriPath(std::string_view const& documentRootPath, std::string_view const& bufferUri)
        {
			// Prevent sAnimatione forms of path traversal from malicious uri references...
            if (bufferUri.empty() || bufferUri.find("..") != std::string::npos || bufferUri.front() == '/' || bufferUri.front() == '\\')
            {
                throw invalid_gltf_document("Invalid buffer.uri value", bufferUri);
            }

            std::string documentRoot = std::string(documentRootPath);
            if (documentRoot.length() > 0)
            {
                if (documentRoot.back() != '/')
                {
                    documentRoot.push_back('/');
                }
            }

            return documentRoot += bufferUri;
        }

    } // namespace detail

    using Attributes = std::unordered_map<std::string, uint32_t>;

	bool Buffer::IsEmbeddedResource() const noexcept
	{
		return uri.find(detail::MimetypeApplicationOctet) == 0;
	}

	void Buffer::SetEmbeddedResource()
	{
		uri = std::string(detail::MimetypeApplicationOctet).append(",").append(base64::Encode(data));
	}


        bool Image::IsEmbeddedResource() const noexcept
        {
            return uri.find(detail::MimetypeImagePNG) == 0 || uri.find(detail::MimetypeImageJPG) == 0;
        }

        void Image::MaterializeData(std::vector<uint8_t> & data) const
        {
            char const * const mimetype = uri.find(detail::MimetypeImagePNG) == 0 ? detail::MimetypeImagePNG : detail::MimetypeImageJPG;
            const std::size_t startPos = std::char_traits<char>::length(mimetype) + 1;
            const std::size_t base64Length = uri.length() - startPos;

#if defined(FX_GLTF_HAS_CPP_17)
            const bool success = base64::TryDecode({ &uri[startPos], base64Length }, data);
#else
            const bool success = base64::TryDecode(uri.substr(startPos), data);
#endif
            if (!success)
            {
                throw invalid_gltf_document("Invalid buffer.uri value", "malformed base64");
            }
        }


    namespace detail
    {


        std::size_t GetFileSize(std::istream & file)
        {
            file.seekg(0, file.end);
            const std::streampos fileSize = file.tellg();
            file.seekg(0, file.beg);

            if (fileSize < 0)
            {
                throw std::system_error(std::make_error_code(std::errc::io_error));
            }

            return static_cast<std::size_t>(fileSize);
        }

         void MaterializeData(Buffer & buffer)
        {
            const std::size_t startPos = std::char_traits<char>::length(detail::MimetypeApplicationOctet) + 1;
            const std::size_t base64Length = buffer.uri.length() - startPos;
            const std::size_t decodedEstimate = base64Length / 4 * 3;
            if ((decodedEstimate - 2) > buffer.byteLength) // we need to give room for padding...
            {
                throw invalid_gltf_document("Invalid buffer.uri value", "malformed base64");
            }

#if defined(FX_GLTF_HAS_CPP_17)
            const bool success = base64::TryDecode({ &buffer.uri[startPos], base64Length }, buffer.data);
#else
            const bool success = base64::TryDecode(buffer.uri.substr(startPos), buffer.data);
#endif
            if (!success)
            {
                throw invalid_gltf_document("Invalid buffer.uri value", "malformed base64");
            }
        }

		void LoadBuffers(std::vector<Buffer> & buffers, DataContext const& dataContext)
        {
				if (buffers.size() > dataContext.readQuotas.MaxBufferCount)
				{
					throw invalid_gltf_document("Quota exceeded : number of buffers > MaxBufferCount");
				}
	
				for (auto & buffer : buffers)
				{
					if (buffer.byteLength == 0)
					{
						throw invalid_gltf_document("Invalid buffer.byteLength value : 0");
					}
	
					if (buffer.byteLength > dataContext.readQuotas.MaxBufferByteLength)
					{
						throw invalid_gltf_document("Quota exceeded : buffer.byteLength > MaxBufferByteLength");
					}
	
					if (!buffer.uri.empty())
					{
						if (buffer.IsEmbeddedResource())
						{
							detail::MaterializeData(buffer);
						}
						else
						{
							std::ifstream fileData(detail::CreateBufferUriPath(dataContext.bufferRootPath, buffer.uri), std::ios::binary);
							if (!fileData.good())
							{
								throw invalid_gltf_document("Invalid buffer.uri value", buffer.uri);
							}
	
							buffer.data.resize(buffer.byteLength);
							fileData.read(reinterpret_cast<char *>(&buffer.data[0]), buffer.byteLength);
						}
					}
					else if (dataContext.binaryData.size())
					{
						detail::ChunkHeader header;
	
						auto binary = dataContext.binaryData;
						std::memcpy(&header, &binary[dataContext.binaryOffset], detail::ChunkHeaderSize);
	
						if (header.chunkType != detail::GLBChunkBIN || header.chunkLength < buffer.byteLength)
						{
							throw invalid_gltf_document("Invalid buffer data");
						}
	
						buffer.data.resize(buffer.byteLength);
						std::memcpy(&buffer.data[0], &binary[dataContext.binaryOffset + detail::ChunkHeaderSize], buffer.byteLength);
					}
				}
        }

         Document Create(nlohmann::json const& json, DataContext const& dataContext, bool skip_buffers)
        {
            Document document = json;

			if(skip_buffers)
				return document;

			LoadBuffers(document.buffers, dataContext);
			return document;
        }

        void ValidateBuffers(const std::vector<Buffer> & buffers, bool useBinaryFormat)
        {
            if (buffers.empty())
            {
                throw invalid_gltf_document("Invalid document. A document must have at least 1 buffer.");
            }

            bool foundBinaryBuffer = false;
            for (std::size_t bufferIndex = 0; bufferIndex < buffers.size(); bufferIndex++)
            {
                Buffer const& buffer = buffers[bufferIndex];
                if (buffer.byteLength == 0)
                {
                    throw invalid_gltf_document("Invalid buffer.byteLength value : 0");
                }

                if (buffer.byteLength != buffer.data.size())
                {
                    throw invalid_gltf_document("Invalid buffer.byteLength value : does not match buffer.data size");
                }

                if (buffer.uri.empty())
                {
                    foundBinaryBuffer = true;
                    if (bufferIndex != 0)
                    {
                        throw invalid_gltf_document("Invalid document. Only 1 buffer, the very first, is allowed to have an empty buffer.uri field.");
                    }
                }
            }

            if (useBinaryFormat && !foundBinaryBuffer)
            {
                throw invalid_gltf_document("Invalid document. No buffer found which can meet the criteria for saving to a .glb file.");
            }
        }

	#ifdef GLTF_SAVE
		std::optional<JsonError> Save(nlohmann::json && json, std::vector<Buffer> const& buffers, std::string const& documentFilePath, bool useBinaryFormat, uint32_t Magic)
        {
			try
			{
				std::size_t externalBufferIndex = 0;
				if (useBinaryFormat)
				{
					detail::GLBHeader header{ Magic, 2, 0, { 0, detail::GLBChunkJSON } };
					detail::ChunkHeader binHeader{ 0, detail::GLBChunkBIN };
	
					std::string jsonText = json.dump();
	
					Buffer const& binBuffer = buffers.front();
					const uint32_t binPaddedLength = ((binBuffer.byteLength + 3) & (~3u));
					const uint32_t binPadding = binPaddedLength - binBuffer.byteLength;
					binHeader.chunkLength = binPaddedLength;
	
					header.jsonHeader.chunkLength = ((jsonText.length() + 3) & (~3u));
					const uint32_t headerPadding = static_cast<uint32_t>(header.jsonHeader.chunkLength - jsonText.length());
					header.length = detail::HeaderSize + header.jsonHeader.chunkLength + detail::ChunkHeaderSize + binHeader.chunkLength;
	
					std::ofstream fileData(documentFilePath, std::ios::binary);
					if (!fileData.good())
					{
						throw std::system_error(std::make_error_code(std::errc::io_error));
					}
	
					const char spaces[3] = { ' ', ' ', ' ' };
					const char nulls[3] = { 0, 0, 0 };
	
					fileData.write(reinterpret_cast<char *>(&header), detail::HeaderSize);
					fileData.write(jsonText.c_str(), jsonText.length());
					fileData.write(&spaces[0], headerPadding);
					fileData.write(reinterpret_cast<char *>(&binHeader), detail::ChunkHeaderSize);
					fileData.write(reinterpret_cast<char const *>(&binBuffer.data[0]), binBuffer.byteLength);
					fileData.write(&nulls[0], binPadding);
	
					fileData.flush();
	
					externalBufferIndex = 1;
				}
				else
				{
					std::ofstream file(documentFilePath);
					if (!file.is_open())
					{
						throw std::system_error(std::make_error_code(std::errc::io_error));
					}
	
					file << json.dump(2);
					file.flush();
				}
	
				// The glTF 2.0 spec allows a document to have more than 1 buffer. However, only the first one will be included in the .glb
				// All others must be considered as External/Embedded resources. Process them if necessary...
				std::string documentRootPath = detail::GetDocumentRootPath(documentFilePath);
				for (; externalBufferIndex < buffers.size(); externalBufferIndex++)
				{
					Buffer const& buffer = buffers[externalBufferIndex];
					if (!buffer.IsEmbeddedResource())
					{
						auto path = detail::CreateBufferUriPath(documentRootPath, buffer.uri);
	
						std::ofstream fileData(detail::CreateBufferUriPath(documentRootPath, buffer.uri), std::ios::binary);
						if (!fileData.good())
						{
							throw invalid_gltf_document("Invalid buffer.uri value", path);
						}
	
						fileData.write(reinterpret_cast<char const *>(&buffer.data[0]), buffer.byteLength);
						fileData.flush();
					}
				}
				
				return {};
			}
			catch (std::exception & e)
			{
				return JsonError{.file=documentFilePath,.what=e.what()};
			}		
        }


		std::optional<JsonError> Save(Document const& document, std::string const& documentFilePath, bool useBinaryFormat)
		{
			try
			{
				nlohmann::json json = document;
				return Save(std::move(json), document.buffers, documentFilePath, useBinaryFormat, GLBHeaderMagic);
			
				return {};
			}
			catch (std::exception & e)
			{
				return JsonError{.file=documentFilePath,.what=e.what()};
			}		
		}
		

#endif
    } // namespace detail

     void from_json(nlohmann::json const& json, Accessor::Type & accessorType)
    {
        std::string type = json.get<std::string>();
        if (type == "SCALAR")
        {
            accessorType = Accessor::Type::Scalar;
        }
        else if (type == "VEC2")
        {
            accessorType = Accessor::Type::Vec2;
        }
        else if (type == "VEC3")
        {
            accessorType = Accessor::Type::Vec3;
        }
        else if (type == "VEC4")
        {
            accessorType = Accessor::Type::Vec4;
        }
        else if (type == "MAT2")
        {
            accessorType = Accessor::Type::Mat2;
        }
        else if (type == "MAT3")
        {
            accessorType = Accessor::Type::Mat3;
        }
        else if (type == "MAT4")
        {
            accessorType = Accessor::Type::Mat4;
        }
        else
        {
            throw invalid_gltf_document("Unknown accessor.type value", type);
        }
    }

     void from_json(nlohmann::json const& json, Accessor::Sparse::Values & values)
    {
        detail::ReadRequiredField("bufferView", json, values.bufferView);

        detail::ReadOptionalField("byteOffset", json, values.byteOffset);

        detail::ReadExtensionsAndExtras(json, values.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Accessor::Sparse::Indices & indices)
    {
        detail::ReadRequiredField("bufferView", json, indices.bufferView);
        detail::ReadRequiredField("componentType", json, indices.componentType);

        detail::ReadOptionalField("byteOffset", json, indices.byteOffset);

        detail::ReadExtensionsAndExtras(json, indices.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Accessor::Sparse & sparse)
    {
        detail::ReadRequiredField("count", json, sparse.count);
        detail::ReadRequiredField("indices", json, sparse.indices);
        detail::ReadRequiredField("values", json, sparse.values);

        detail::ReadExtensionsAndExtras(json, sparse.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Accessor & accessor)
    {
        detail::ReadRequiredField("componentType", json, accessor.componentType);
        detail::ReadRequiredField("count", json, accessor.count);
        detail::ReadRequiredField("type", json, accessor.type);

        detail::ReadOptionalField("bufferView", json, accessor.bufferView);
        detail::ReadOptionalField("byteOffset", json, accessor.byteOffset);
        detail::ReadOptionalField("max", json, accessor.max);
        detail::ReadOptionalField("min", json, accessor.min);
        detail::ReadOptionalField("name", json, accessor.name);
        detail::ReadOptionalField("normalized", json, accessor.normalized);
        detail::ReadOptionalField("sparse", json, accessor.sparse);

        detail::ReadExtensionsAndExtras(json, accessor.extensionsAndExtras);
    }

	 void from_json(nlohmann::json const& json, Animation::Channel::Target & animationChannelTarget)
    {
        detail::ReadRequiredField("path", json, animationChannelTarget.path);

        detail::ReadOptionalField("node", json, animationChannelTarget.node);

        detail::ReadExtensionsAndExtras(json, animationChannelTarget.extensionsAndExtras);
    }

	 void from_json(nlohmann::json const& json, Animation::Channel::Target::Path & animationTargetType)
    {
        std::string type = json.get<std::string>();
        if (type == "translation")
        {
			animationTargetType = Animation::Channel::Target::Path::Translation;
        }
        else if (type == "rotation")
        {
			animationTargetType = Animation::Channel::Target::Path::Rotation;
        }
        else if (type == "scale")
        {
			animationTargetType = Animation::Channel::Target::Path::Scale;
        }
		else if (type == "weights")
        {
			animationTargetType = Animation::Channel::Target::Path::Weights;
        }
        else
        {
            throw invalid_gltf_document("Unknown animation.sampler.interpolation value", type);
        }
    }

	 void from_json(nlohmann::json const& json, Animation::Channel & animationChannel)
    {
        detail::ReadRequiredField("sampler", json, animationChannel.sampler);
        detail::ReadRequiredField("target", json, animationChannel.target);

        detail::ReadExtensionsAndExtras(json, animationChannel.extensionsAndExtras);
    }

	 void from_json(nlohmann::json const& json, Animation::Sampler::Type & animationSamplerType)
    {
        std::string type = json.get<std::string>();
        if (type == "LINEAR")
        {
			animationSamplerType = Animation::Sampler::Type::Linear;
        }
        else if (type == "STEP")
        {
			animationSamplerType = Animation::Sampler::Type::Step;
        }
        else if (type == "CUBICSPLINE")
        {
			animationSamplerType = Animation::Sampler::Type::CubicSpline;
        }
        else
        {
            throw invalid_gltf_document("Unknown animation.sampler.interpolation value", type);
        }
    }

	 void from_json(nlohmann::json const& json, Animation::Sampler & animationSampler)
    {
        detail::ReadRequiredField("input", json, animationSampler.input);
        detail::ReadRequiredField("output", json, animationSampler.output);

        detail::ReadOptionalField("interpolation", json, animationSampler.interpolation);

        detail::ReadExtensionsAndExtras(json, animationSampler.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Animation & animation)
    {
        detail::ReadRequiredField("channels", json, animation.channels);
        detail::ReadRequiredField("samplers", json, animation.samplers);

        detail::ReadOptionalField("name", json, animation.name);

        detail::ReadExtensionsAndExtras(json, animation.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Asset & asset)
    {
        detail::ReadRequiredField("version", json, asset.version);
        detail::ReadOptionalField("copyright", json, asset.copyright);
        detail::ReadOptionalField("generator", json, asset.generator);
        detail::ReadOptionalField("minVersion", json, asset.minVersion);

        detail::ReadExtensionsAndExtras(json, asset.extensionsAndExtras);
    }


     void from_json(nlohmann::json const& json, BufferView & bufferView)
    {
        detail::ReadRequiredField("buffer", json, bufferView.buffer);
        detail::ReadRequiredField("byteLength", json, bufferView.byteLength);

        detail::ReadOptionalField("byteOffset", json, bufferView.byteOffset);
        detail::ReadOptionalField("byteStride", json, bufferView.byteStride);
        detail::ReadOptionalField("name", json, bufferView.name);
        detail::ReadOptionalField("target", json, bufferView.target);

        detail::ReadExtensionsAndExtras(json, bufferView.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Camera::Type & cameraType)
    {
        std::string type = json.get<std::string>();
        if (type == "orthographic")
        {
            cameraType = Camera::Type::Orthographic;
        }
        else if (type == "perspective")
        {
            cameraType = Camera::Type::Perspective;
        }
        else
        {
            throw invalid_gltf_document("Unknown camera.type value", type);
        }
    }

     void from_json(nlohmann::json const& json, Camera::Orthographic & camera)
    {
        detail::ReadRequiredField("xmag", json, camera.xmag);
        detail::ReadRequiredField("ymag", json, camera.ymag);
        detail::ReadRequiredField("zfar", json, camera.zfar);
        detail::ReadRequiredField("znear", json, camera.znear);

        detail::ReadExtensionsAndExtras(json, camera.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Camera::Perspective & camera)
    {
        detail::ReadRequiredField("yfov", json, camera.yfov);
        detail::ReadRequiredField("znear", json, camera.znear);

        detail::ReadOptionalField("aspectRatio", json, camera.aspectRatio);
        detail::ReadOptionalField("zfar", json, camera.zfar);

        detail::ReadExtensionsAndExtras(json, camera.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Camera & camera)
    {
        detail::ReadRequiredField("type", json, camera.type);

        detail::ReadOptionalField("name", json, camera.name);

        detail::ReadExtensionsAndExtras(json, camera.extensionsAndExtras);

        if (camera.type == Camera::Type::Perspective)
        {
            detail::ReadRequiredField("perspective", json, camera.perspective);
        }
        else if (camera.type == Camera::Type::Orthographic)
        {
            detail::ReadRequiredField("orthographic", json, camera.orthographic);
        }
	}

     void from_json(nlohmann::json const& json, Image & image)
    {
        detail::ReadOptionalField("bufferView", json, image.bufferView);
        detail::ReadOptionalField("mimeType", json, image.mimeType);
        detail::ReadOptionalField("name", json, image.name);
        detail::ReadOptionalField("uri", json, image.uri);

		image.uri = decodeURIPercentEncoding(image.uri);

        detail::ReadExtensionsAndExtras(json, image.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Material::AlphaMode & materialAlphaMode)
    {
        std::string alphaMode = json.get<std::string>();
        if (alphaMode == "OPAQUE")
        {
            materialAlphaMode = Material::AlphaMode::Opaque;
        }
        else if (alphaMode == "MASK")
        {
            materialAlphaMode = Material::AlphaMode::Mask;
        }
        else if (alphaMode == "BLEND")
        {
            materialAlphaMode = Material::AlphaMode::Blend;
        }
        else
        {
            throw invalid_gltf_document("Unknown material.alphaMode value", alphaMode);
        }
    }

    void from_json(nlohmann::json const& json, Material::Texture & materialTexture)
    {
        detail::ReadRequiredField("index", json, materialTexture.index);
        detail::ReadOptionalField("texCoord", json, materialTexture.texCoord);

        detail::ReadExtensionsAndExtras(json, materialTexture.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Material::NormalTexture & materialTexture)
    {
        from_json(json, static_cast<Material::Texture &>(materialTexture));
        detail::ReadOptionalField("scale", json, materialTexture.scale);

        detail::ReadExtensionsAndExtras(json, materialTexture.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Material::OcclusionTexture & materialTexture)
    {
        from_json(json, static_cast<Material::Texture &>(materialTexture));
        detail::ReadOptionalField("strength", json, materialTexture.strength);

        detail::ReadExtensionsAndExtras(json, materialTexture.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Material::PBRMetallicRoughness & pbrMetallicRoughness)
    {
        detail::ReadOptionalField("baseColorFactor", json, pbrMetallicRoughness.baseColorFactor);
        detail::ReadOptionalField("baseColorTexture", json, pbrMetallicRoughness.baseColorTexture);
        detail::ReadOptionalField("metallicFactor", json, pbrMetallicRoughness.metallicFactor);
        detail::ReadOptionalField("metallicRoughnessTexture", json, pbrMetallicRoughness.metallicRoughnessTexture);
        detail::ReadOptionalField("roughnessFactor", json, pbrMetallicRoughness.roughnessFactor);

        detail::ReadExtensionsAndExtras(json, pbrMetallicRoughness.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Material & material)
    {
        detail::ReadOptionalField("alphaMode", json, material.alphaMode);
        detail::ReadOptionalField("alphaCutoff", json, material.alphaCutoff);
        detail::ReadOptionalField("doubleSided", json, material.doubleSided);
        detail::ReadOptionalField("emissiveFactor", json, material.emissiveFactor);
        detail::ReadOptionalField("emissiveTexture", json, material.emissiveTexture);
        detail::ReadOptionalField("name", json, material.name);
        detail::ReadOptionalField("normalTexture", json, material.normalTexture);
        detail::ReadOptionalField("occlusionTexture", json, material.occlusionTexture);
        detail::ReadOptionalField("pbrMetallicRoughness", json, material.pbrMetallicRoughness);

        detail::ReadExtensionsAndExtras(json, material.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Mesh & mesh)
    {
        detail::ReadRequiredField("primitives", json, mesh.primitives);

        detail::ReadOptionalField("name", json, mesh.name);
        detail::ReadOptionalField("weights", json, mesh.weights);

        detail::ReadExtensionsAndExtras(json, mesh.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Node & node)
    {
        detail::ReadOptionalField("camera", json, node.camera);
        detail::ReadOptionalField("children", json, node.children);
        detail::ReadOptionalField("matrix", json, node.matrix);
        detail::ReadOptionalField("mesh", json, node.mesh);
        detail::ReadOptionalField("name", json, node.name);
        detail::ReadOptionalField("rotation", json, node.rotation);
        detail::ReadOptionalField("scale", json, node.scale);
        detail::ReadOptionalField("skin", json, node.skin);
        detail::ReadOptionalField("translation", json, node.translation);

        detail::ReadExtensionsAndExtras(json, node.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Primitive & primitive)
    {
        detail::ReadRequiredField("attributes", json, primitive.attributes);

        detail::ReadOptionalField("indices", json, primitive.indices);
        detail::ReadOptionalField("material", json, primitive.material);
        detail::ReadOptionalField("mode", json, primitive.mode);
        detail::ReadOptionalField("targets", json, primitive.targets);

        detail::ReadExtensionsAndExtras(json, primitive.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Sampler & sampler)
    {
        detail::ReadOptionalField("magFilter", json, sampler.magFilter);
        detail::ReadOptionalField("minFilter", json, sampler.minFilter);
        detail::ReadOptionalField("name", json, sampler.name);
        detail::ReadOptionalField("wrapS", json, sampler.wrapS);
        detail::ReadOptionalField("wrapT", json, sampler.wrapT);

        detail::ReadExtensionsAndExtras(json, sampler.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Scene & scene)
    {
        detail::ReadOptionalField("name", json, scene.name);
        detail::ReadOptionalField("nodes", json, scene.nodes);

        detail::ReadExtensionsAndExtras(json, scene.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Skin & skin)
    {
        detail::ReadRequiredField("joints", json, skin.joints);

        detail::ReadOptionalField("inverseBindMatrices", json, skin.inverseBindMatrices);
        detail::ReadOptionalField("name", json, skin.name);
        detail::ReadOptionalField("skeleton", json, skin.skeleton);

        detail::ReadExtensionsAndExtras(json, skin.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Texture & texture)
    {
        detail::ReadOptionalField("name", json, texture.name);
        detail::ReadOptionalField("sampler", json, texture.sampler);
        detail::ReadOptionalField("source", json, texture.source);
#ifdef 	CHEETAH
		detail::ReadOptionalField("texCoords", json, texture.texCoords);
#endif

        detail::ReadExtensionsAndExtras(json, texture.extensionsAndExtras);
    }

     void from_json(nlohmann::json const& json, Document & document)
    {
        detail::ReadRequiredField("asset", json, document.asset);

        detail::ReadOptionalField("accessors", json, document.accessors);
        detail::ReadOptionalField("animations", json, document.animations);
        detail::ReadOptionalField("buffers", json, document.buffers);
        detail::ReadOptionalField("bufferViews", json, document.bufferViews);
        detail::ReadOptionalField("cameras", json, document.cameras);
        detail::ReadOptionalField("materials", json, document.materials);
        detail::ReadOptionalField("meshes", json, document.meshes);
        detail::ReadOptionalField("nodes", json, document.nodes);
        detail::ReadOptionalField("images", json, document.images);
        detail::ReadOptionalField("samplers", json, document.samplers);
        detail::ReadOptionalField("scene", json, document.scene);
        detail::ReadOptionalField("scenes", json, document.scenes);
        detail::ReadOptionalField("skins", json, document.skins);
        detail::ReadOptionalField("textures", json, document.textures);

        detail::ReadOptionalField("extensionsUsed", json, document.extensionsUsed);
        detail::ReadOptionalField("extensionsRequired", json, document.extensionsRequired);
        detail::ReadExtensionsAndExtras(json, document.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Accessor::ComponentType const& accessorComponentType)
    {
        if (accessorComponentType == Accessor::ComponentType::None)
        {
            throw invalid_gltf_document("Unknown accessor.componentType value");
        }

        json = static_cast<uint16_t>(accessorComponentType);
    }

     void to_json(nlohmann::json & json, Accessor::Type const& accessorType)
    {
        switch (accessorType)
        {
        case Accessor::Type::Scalar:
            json = "SCALAR";
            break;
        case Accessor::Type::Vec2:
            json = "VEC2";
            break;
        case Accessor::Type::Vec3:
            json = "VEC3";
            break;
        case Accessor::Type::Vec4:
            json = "VEC4";
            break;
        case Accessor::Type::Mat2:
            json = "MAT2";
            break;
        case Accessor::Type::Mat3:
            json = "MAT3";
            break;
        case Accessor::Type::Mat4:
            json = "MAT4";
            break;
        default:
            throw invalid_gltf_document("Unknown accessor.type value");
        }
    }

     void to_json(nlohmann::json & json, Accessor::Sparse::Values const& values)
    {
        detail::WriteField("bufferView", json, values.bufferView, static_cast<uint32_t>(-1));
        detail::WriteField("byteOffset", json, values.byteOffset, {});
        detail::WriteExtensions(json, values.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Accessor::Sparse::Indices const& indices)
    {
        detail::WriteField("componentType", json, indices.componentType, Accessor::ComponentType::None);
        detail::WriteField("bufferView", json, indices.bufferView, static_cast<uint32_t>(-1));
        detail::WriteField("byteOffset", json, indices.byteOffset, {});
        detail::WriteExtensions(json, indices.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Accessor::Sparse const& sparse)
    {
        detail::WriteField("count", json, sparse.count, -1);
        detail::WriteField("indices", json, sparse.indices);
        detail::WriteField("values", json, sparse.values);
        detail::WriteExtensions(json, sparse.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Accessor const& accessor)
    {
        detail::WriteField("bufferView", json, accessor.bufferView, -1);
        detail::WriteField("byteOffset", json, accessor.byteOffset, {});
        detail::WriteField("componentType", json, accessor.componentType, Accessor::ComponentType::None);
        detail::WriteField("count", json, accessor.count, {});
        detail::WriteField("max", json, accessor.max);
        detail::WriteField("min", json, accessor.min);
        detail::WriteField("name", json, accessor.name);
        detail::WriteField("normalized", json, accessor.normalized, false);
        detail::WriteField("sparse", json, accessor.sparse);
        detail::WriteField("type", json, accessor.type, Accessor::Type::None);
        detail::WriteExtensions(json, accessor.extensionsAndExtras);
    }

	 void to_json(nlohmann::json & json, Animation::Channel::Target::Path const& animationTargetPath)
    {
        switch (animationTargetPath)
        {
		case Animation::Channel::Target::Path::Translation:
            json = "translation";
            break;
		case Animation::Channel::Target::Path::Rotation:
            json = "rotation";
            break;
		case Animation::Channel::Target::Path::Scale:
            json = "scale";
            break;
		case Animation::Channel::Target::Path::Weights:
            json = "weights";
            break;
		default:
            throw invalid_gltf_document("Unknown animation.targetPath value");
        }
    }

	 void to_json(nlohmann::json & json, Animation::Channel::Target const& animationChannelTarget)
    {
        detail::WriteField("node", json, animationChannelTarget.node, -1);
		detail::WriteField("path", json, animationChannelTarget.path, Animation::Channel::Target::Path::Undefined);
        detail::WriteExtensions(json, animationChannelTarget.extensionsAndExtras);
    }

	 void to_json(nlohmann::json & json, Animation::Channel const& animationChannel)
    {
        detail::WriteField("sampler", json, animationChannel.sampler, -1);
        detail::WriteField("target", json, animationChannel.target);
        detail::WriteExtensions(json, animationChannel.extensionsAndExtras);
    }

	 void to_json(nlohmann::json & json, Animation::Sampler::Type const& animationSamplerType)
    {
        switch (animationSamplerType)
        {
		case Animation::Sampler::Type::Linear:
            json = "LINEAR";
            break;
		case Animation::Sampler::Type::Step:
            json = "STEP";
            break;
		case Animation::Sampler::Type::CubicSpline:
            json = "CUBICSPLINE";
            break;
        }
    }

	 void to_json(nlohmann::json & json, Animation::Sampler const& animationSampler)
    {
        detail::WriteField("input", json, animationSampler.input, -1);
		detail::WriteField("interpolation", json, animationSampler.interpolation, Animation::Sampler::Type::Linear);
        detail::WriteField("output", json, animationSampler.output, -1);
        detail::WriteExtensions(json, animationSampler.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Animation const& animation)
    {
        detail::WriteField("channels", json, animation.channels);
        detail::WriteField("name", json, animation.name);
        detail::WriteField("samplers", json, animation.samplers);
        detail::WriteExtensions(json, animation.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Asset const& asset)
    {
        detail::WriteField("copyright", json, asset.copyright);
        detail::WriteField("generator", json, asset.generator);
        detail::WriteField("minVersion", json, asset.minVersion);
        detail::WriteField("version", json, asset.version);
        detail::WriteExtensions(json, asset.extensionsAndExtras);
    }

	 void from_json(nlohmann::json const& json, Buffer & buffer)
    {
        detail::ReadRequiredField("byteLength", json, buffer.byteLength);

        detail::ReadOptionalField("name", json, buffer.name);
		detail::ReadOptionalField("uri", json, buffer.uri);

		buffer.uri = decodeURIPercentEncoding(buffer.uri);

        detail::ReadExtensionsAndExtras(json, buffer.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Buffer const& buffer)
    {
        detail::WriteField("byteLength", json, buffer.byteLength, {});
        detail::WriteField("name", json, buffer.name);
        detail::WriteField("uri", json, buffer.uri);
        detail::WriteExtensions(json, buffer.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, BufferView const& bufferView)
    {
        detail::WriteField("buffer", json, bufferView.buffer, -1);
        detail::WriteField("byteLength", json, bufferView.byteLength, {});
        detail::WriteField("byteOffset", json, bufferView.byteOffset, {});
        detail::WriteField("byteStride", json, bufferView.byteStride, {});
        detail::WriteField("name", json, bufferView.name);
        detail::WriteField("target", json, bufferView.target, BufferView::TargetType::None);
        detail::WriteExtensions(json, bufferView.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Camera::Type const& cameraType)
    {
        switch (cameraType)
        {
        case Camera::Type::Orthographic:
            json = "orthographic";
            break;
        case Camera::Type::Perspective:
            json = "perspective";
            break;
        default:
            throw invalid_gltf_document("Unknown camera.type value");
        }
    }

     void to_json(nlohmann::json & json, Camera::Orthographic const& camera)
    {
        detail::WriteField("xmag", json, camera.xmag, defaults::FloatSentinel);
        detail::WriteField("ymag", json, camera.ymag, defaults::FloatSentinel);
        detail::WriteField("zfar", json, camera.zfar, -defaults::FloatSentinel);
        detail::WriteField("znear", json, camera.znear, -defaults::FloatSentinel);
        detail::WriteExtensions(json, camera.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Camera::Perspective const& camera)
    {
        detail::WriteField("aspectRatio", json, camera.aspectRatio, {});
        detail::WriteField("yfov", json, camera.yfov, {});
        detail::WriteField("zfar", json, camera.zfar, {});
        detail::WriteField("znear", json, camera.znear, {});
        detail::WriteExtensions(json, camera.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Camera const& camera)
    {
        detail::WriteField("name", json, camera.name);
        detail::WriteField("type", json, camera.type, Camera::Type::None);
        detail::WriteExtensions(json, camera.extensionsAndExtras);

        if (camera.type == Camera::Type::Perspective)
        {
            detail::WriteField("perspective", json, camera.perspective);
        }
        else if (camera.type == Camera::Type::Orthographic)
        {
            detail::WriteField("orthographic", json, camera.orthographic);
        }
    }

     void to_json(nlohmann::json & json, Image const& image)
    {
        detail::WriteField("bufferView", json, image.bufferView, image.uri.empty() ? -1 : 0); // bufferView or uri need to be written; even if default 0
        detail::WriteField("mimeType", json, image.mimeType);
        detail::WriteField("name", json, image.name);
        detail::WriteField("uri", json, image.uri);
        detail::WriteExtensions(json, image.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Material::AlphaMode const& materialAlphaMode)
    {
        switch (materialAlphaMode)
        {
        case Material::AlphaMode::Opaque:
            json = "OPAQUE";
            break;
        case Material::AlphaMode::Mask:
            json = "MASK";
            break;
        case Material::AlphaMode::Blend:
            json = "BLEND";
            break;
        }
    }

    void to_json(nlohmann::json & json, Material::Texture const& materialTexture)
    {
        detail::WriteField("index", json, materialTexture.index, -1);
        detail::WriteField("texCoord", json, materialTexture.texCoord, 0);
        detail::WriteExtensions(json, materialTexture.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Material::NormalTexture const& materialTexture)
    {
        to_json(json, static_cast<Material::Texture const&>(materialTexture));
        detail::WriteField("scale", json, materialTexture.scale, defaults::IdentityScalar);
        detail::WriteExtensions(json, materialTexture.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Material::OcclusionTexture const& materialTexture)
    {
        to_json(json, static_cast<Material::Texture const&>(materialTexture));
        detail::WriteField("strength", json, materialTexture.strength, defaults::IdentityScalar);
        detail::WriteExtensions(json, materialTexture.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Material::PBRMetallicRoughness const& pbrMetallicRoughness)
    {
        detail::WriteField("baseColorFactor", json, pbrMetallicRoughness.baseColorFactor, defaults::IdentityVec4);
        detail::WriteField("baseColorTexture", json, pbrMetallicRoughness.baseColorTexture);
        detail::WriteField("metallicFactor", json, pbrMetallicRoughness.metallicFactor, defaults::IdentityScalar);
        detail::WriteField("metallicRoughnessTexture", json, pbrMetallicRoughness.metallicRoughnessTexture);
        detail::WriteField("roughnessFactor", json, pbrMetallicRoughness.roughnessFactor, defaults::IdentityScalar);
        detail::WriteExtensions(json, pbrMetallicRoughness.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Material const& material)
    {
        detail::WriteField("alphaCutoff", json, material.alphaCutoff, defaults::MaterialAlphaCutoff);
        detail::WriteField("alphaMode", json, material.alphaMode, Material::AlphaMode::Opaque);
        detail::WriteField("doubleSided", json, material.doubleSided, defaults::MaterialDoubleSided);
        detail::WriteField("emissiveTexture", json, material.emissiveTexture);
        detail::WriteField("emissiveFactor", json, material.emissiveFactor, defaults::NullVec3);
        detail::WriteField("name", json, material.name);
        detail::WriteField("normalTexture", json, material.normalTexture);
        detail::WriteField("occlusionTexture", json, material.occlusionTexture);
        detail::WriteField("pbrMetallicRoughness", json, material.pbrMetallicRoughness);

        detail::WriteExtensions(json, material.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Mesh const& mesh)
    {
        detail::WriteField("name", json, mesh.name);
        detail::WriteField("primitives", json, mesh.primitives);
        detail::WriteField("weights", json, mesh.weights);
        detail::WriteExtensions(json, mesh.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Node const& node)
    {
        detail::WriteField("camera", json, node.camera, -1);
        detail::WriteField("children", json, node.children);
        detail::WriteField("matrix", json, node.matrix, defaults::IdentityMatrix);
        detail::WriteField("mesh", json, node.mesh, -1);
        detail::WriteField("name", json, node.name);
        detail::WriteField("rotation", json, node.rotation, defaults::IdentityRotation);
        detail::WriteField("scale", json, node.scale, defaults::IdentityVec3);
        detail::WriteField("skin", json, node.skin, -1);
        detail::WriteField("translation", json, node.translation, defaults::NullVec3);
        detail::WriteField("weights", json, node.weights);
        detail::WriteExtensions(json, node.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Primitive const& primitive)
    {
        detail::WriteField("attributes", json, primitive.attributes);
        detail::WriteField("indices", json, primitive.indices, -1);
        detail::WriteField("material", json, primitive.material, -1);
        detail::WriteField("mode", json, primitive.mode, Primitive::Mode::Triangles);
        detail::WriteField("targets", json, primitive.targets);
        detail::WriteExtensions(json, primitive.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Sampler const& sampler)
    {
        if (!sampler.empty())
        {
            detail::WriteField("name", json, sampler.name);
            detail::WriteField("magFilter", json, sampler.magFilter, Sampler::MagFilter::None);
            detail::WriteField("minFilter", json, sampler.minFilter, Sampler::MinFilter::None);
            detail::WriteField("wrapS", json, sampler.wrapS, Sampler::WrappingMode::Repeat);
            detail::WriteField("wrapT", json, sampler.wrapT, Sampler::WrappingMode::Repeat);
            detail::WriteExtensions(json, sampler.extensionsAndExtras);
        }
        else
        {
            // If a sampler is completely empty we still need to write out an empty object for the encompassing array...
            json = nlohmann::json::object();
        }
    }

     void to_json(nlohmann::json & json, Scene const& scene)
    {
        detail::WriteField("name", json, scene.name);
        detail::WriteField("nodes", json, scene.nodes);
        detail::WriteExtensions(json, scene.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Skin const& skin)
    {
        detail::WriteField("inverseBindMatrices", json, skin.inverseBindMatrices, -1);
        detail::WriteField("name", json, skin.name);
        detail::WriteField("skeleton", json, skin.skeleton, -1);
        detail::WriteField("joints", json, skin.joints);
        detail::WriteExtensions(json, skin.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Texture const& texture)
    {
        detail::WriteField("name", json, texture.name);
        detail::WriteField("sampler", json, texture.sampler, -1);
        detail::WriteField("source", json, texture.source, -1);
#ifdef 	CHEETAH
		detail::WriteField("texCoords", json, texture.texCoords, -1);
#endif

        detail::WriteExtensions(json, texture.extensionsAndExtras);
    }

     void to_json(nlohmann::json & json, Document const& document)
    {
        detail::WriteField("accessors", json, document.accessors);
        detail::WriteField("animations", json, document.animations);
        detail::WriteField("asset", json, document.asset);
        detail::WriteField("buffers", json, document.buffers);
        detail::WriteField("bufferViews", json, document.bufferViews);
        detail::WriteField("cameras", json, document.cameras);
        detail::WriteField("images", json, document.images);
        detail::WriteField("materials", json, document.materials);
        detail::WriteField("meshes", json, document.meshes);
        detail::WriteField("nodes", json, document.nodes);
        detail::WriteField("samplers", json, document.samplers);
        detail::WriteField("scene", json, document.scene, -1);
        detail::WriteField("scenes", json, document.scenes);
        detail::WriteField("skins", json, document.skins);
        detail::WriteField("textures", json, document.textures);

        detail::WriteField("extensionsUsed", json, document.extensionsUsed);
        detail::WriteField("extensionsRequired", json, document.extensionsRequired);
        detail::WriteExtensions(json, document.extensionsAndExtras);
    }

    tl::expected<Document, JsonError> LoadFromText(std::string const& documentFilePath, bool skip_buffers, ReadQuotas const& readQuotas)
    {
        try
        {
			nlohmann::json json;
            {
                std::ifstream file(documentFilePath);
                if (!file.is_open())
                {
                    throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory));
                }

                file >> json;
            }

            auto doc = detail::Create(json, { detail::GetDocumentRootPath(documentFilePath), readQuotas }, skip_buffers);
			doc.name = documentFilePath;
			return doc;
        }
        catch (std::exception & e)
        {
			return tl::unexpected(JsonError{.file=documentFilePath,.what=e.what()});
        }
    }

	std::optional<JsonError> LoadExternalBuffers(Document & document, std::string const& documentFilePath, ReadQuotas const& readQuotas)
    {
		try
        {
			detail::LoadBuffers(document.buffers, { detail::GetDocumentRootPath(documentFilePath), readQuotas });
			return {};
		}
        catch (std::exception & e)
        {
			return JsonError{.file=documentFilePath,.what=e.what()};
        }
	}

    tl::expected<Document, JsonError> LoadFromBinary(std::vector<uint8_t> binary, std::string const& documentFilePath, bool skip_buffers, ReadQuotas const& readQuotas)
    {
        try
        {
            detail::GLBHeader header;
            std::memcpy(&header, &binary[0], detail::HeaderSize);

            bool const isCbor = (header.magic == detail::GLBHeaderMagicCBOR);
            if ((!isCbor && header.magic != detail::GLBHeaderMagic) ||
                header.jsonHeader.chunkType != detail::GLBChunkJSON ||
                header.jsonHeader.chunkLength + detail::HeaderSize > header.length)
            {
                throw invalid_gltf_document("Invalid GLB header");
            }

            // Only the structural decode differs; BIN framing (DataContext.binaryOffset
            // below) is identical for both, since chunkLength is the structural chunk's
            // byte count either way.
            nlohmann::json structural = isCbor
                ? nlohmann::json::from_cbor(binary.begin() + detail::HeaderSize,
                                            binary.begin() + detail::HeaderSize + header.jsonHeader.chunkLength)
                : nlohmann::json::parse({ &binary[detail::HeaderSize], header.jsonHeader.chunkLength });

            auto doc = detail::Create(
                std::move(structural),
                { detail::GetDocumentRootPath(documentFilePath), readQuotas, binary, header.jsonHeader.chunkLength + detail::HeaderSize },
				skip_buffers);

			doc.name = documentFilePath;
			return doc;
        }
        catch (std::exception & e)
        {
			return tl::unexpected(JsonError{.file=documentFilePath,.what=e.what()});
        }
    }

	tl::expected<Document, JsonError> LoadFromBinary(std::string const& documentFilePath, bool skip_buffers, ReadQuotas const& readQuotas)
	{
		try
		{
			std::vector<uint8_t> binary{};
			{
				std::ifstream file(documentFilePath, std::ios::binary);
				if (!file.is_open())
				{
					throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory), documentFilePath);
				}
	
				const std::size_t fileSize = detail::GetFileSize(file);
				if (fileSize < detail::HeaderSize)
				{
					throw invalid_gltf_document("Invalid GLB file");
				}
	
				if (fileSize > readQuotas.MaxFileSize)
				{
					throw invalid_gltf_document("Quota exceeded : file size > MaxFileSize");
				}
	
				binary.resize(fileSize);
				file.read(reinterpret_cast<char *>(&binary[0]), fileSize);
			}
	
			return LoadFromBinary(binary, documentFilePath, skip_buffers, readQuotas);
        }
        catch (std::exception & e)
        {
			return tl::unexpected(JsonError{.file=documentFilePath,.what=e.what()});
        }
	}


	 tl::expected<Document, JsonError> Load(const std::filesystem::path &documentFilePath)
	{
		try
		{
			std::ifstream stream(documentFilePath.string(), std::ios::binary);
			
			if (!stream.is_open())
			{
				throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory), documentFilePath);
			}
	
			auto start = stream.tellg();
			stream.seekg(0, std::ios::end);
			auto size = stream.tellg();
			stream.seekg(start, std::ios::beg);
	
			if(size < 4)
				return {};

			auto documentRootPath = documentFilePath.parent_path().string();

			uint32_t magic;
			auto pos = stream.tellg();
			stream.read((char*)&magic, 4);
			stream.seekg(pos);

			if(magic == fx::gltf::detail::GLBHeaderMagic)
			{
				auto doc = fx::gltf::LoadFromBinary(stream, documentRootPath);
				return doc;
			}


			auto doc = fx::gltf::LoadFromText(stream, documentRootPath);
			return doc;
		}
        catch (std::exception & e)
        {
			return tl::unexpected(JsonError{.file=documentFilePath,.what=e.what()});
        }
	}

	tl::expected<Document, JsonError> LoadFromText(std::istream & input, std::string const & documentRootPath, ReadQuotas const & readQuotas)
	{	
		try
		{
			nlohmann::json json;
            {
                input >> json;
            }

			auto doc = detail::Create(json, { (documentRootPath), readQuotas }, false);
			doc.name = documentRootPath;
			return doc;
        }
        catch (std::exception & e)
        {
			return tl::unexpected(JsonError{.file=documentRootPath,.what=e.what()});
        }	
	}
	
	tl::expected<Document, JsonError> LoadFromBinary(std::istream & input, std::string const & documentRootPath, ReadQuotas const & readQuotas)
	{
		try
		{
			std::vector<uint8_t> binary{};
			{
				if (input.bad())
				{
					throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory), documentRootPath);
				}
	
				const std::size_t fileSize = detail::GetFileSize(input);
				if (fileSize < detail::HeaderSize)
				{
					throw invalid_gltf_document("Invalid GLB file");
				}
	
				if (fileSize > readQuotas.MaxFileSize)
				{
					throw invalid_gltf_document("Quota exceeded : file size > MaxFileSize");
				}
	
				binary.resize(fileSize);
				input.read(reinterpret_cast<char *>(&binary[0]), fileSize);
			}
	
			return LoadFromBinary(binary, documentRootPath, false, readQuotas);	
        }
        catch (std::exception & e)
        {
			return tl::unexpected(JsonError{.file=documentRootPath,.what=e.what()});
        }		
	}
	
	std::optional<JsonError> Save(Document const & document, std::ostream & output, std::string const & documentRootPath, bool useBinaryFormat, bool useCbor)
	{
		try
		{
			// There is no way to check if an ostream has been opened in binary mode or not. Just checking
			// if it's "good" is the best we can do from here...
			nlohmann::json json = document;

			std::size_t externalBufferIndex = 0;
			if (useBinaryFormat)
			{
				detail::GLBHeader header{ useCbor ? detail::GLBHeaderMagicCBOR : detail::GLBHeaderMagic,
										  2, 0, { 0, detail::GLBChunkJSON } };
				detail::ChunkHeader binHeader{ 0, detail::GLBChunkBIN };

				// Structural chunk: CBOR bytes or JSON text.
				std::vector<uint8_t> cborBytes;
				std::string          jsonText;
				uint32_t             structuralLen;
				if (useCbor) { cborBytes = nlohmann::json::to_cbor(json); structuralLen = static_cast<uint32_t>(cborBytes.size()); }
				else         { jsonText  = json.dump();                    structuralLen = static_cast<uint32_t>(jsonText.length()); }

				Buffer const & binBuffer = document.buffers.front();
				const uint32_t binPaddedLength = ((binBuffer.byteLength + 3) & (~3u));
				const uint32_t binPadding = binPaddedLength - binBuffer.byteLength;
				binHeader.chunkLength = binPaddedLength;

				header.jsonHeader.chunkLength = ((structuralLen + 3) & (~3u));
				const uint32_t headerPadding = header.jsonHeader.chunkLength - structuralLen;
				header.length = detail::HeaderSize + header.jsonHeader.chunkLength + detail::ChunkHeaderSize + binHeader.chunkLength;

				constexpr std::array<char, 3> spaces = { ' ', ' ', ' ' };
				constexpr std::array<char, 3> nulls = { 0, 0, 0 };

				output.write(reinterpret_cast<char *>(&header), detail::HeaderSize);
				if (useCbor)
				{
					output.write(reinterpret_cast<char const *>(cborBytes.data()), structuralLen);
					output.write(&nulls[0], headerPadding);   // pad CBOR with zeros, never spaces
				}
				else
				{
					output.write(jsonText.c_str(), structuralLen);
					output.write(&spaces[0], headerPadding);  // JSON pads with spaces (valid whitespace)
				}
				output.write(reinterpret_cast<char *>(&binHeader), detail::ChunkHeaderSize);
				output.write(reinterpret_cast<char const *>(&binBuffer.data[0]), binBuffer.byteLength);
				output.write(&nulls[0], binPadding);

				externalBufferIndex = 1;
			}
			else
			{
				output << json.dump(2);
			}
			
			// The glTF 2.0 spec allows a document to have more than 1 buffer. However, only the first one will be included in the .glb
			// All others must be considered as External/Embedded resources. Process them if necessary...
			for (; externalBufferIndex < document.buffers.size(); externalBufferIndex++)
			{
				Buffer const & buffer = document.buffers[externalBufferIndex];
				if (!buffer.IsEmbeddedResource())
				{
					auto path = detail::CreateBufferUriPath(documentRootPath, buffer.uri);
	
					std::ofstream fileData(path, std::ios::binary);
					if (!fileData.good())
					{
						throw invalid_gltf_document("Invalid buffer.uri value", path);
					}
					
					fileData.write(reinterpret_cast<char const *>(&buffer.data[0]), buffer.byteLength);
				}
			}
			
			return {};
        }
        catch (std::exception & e)
        {
			return JsonError{.file=documentRootPath,.what=e.what()};
        }		
	}
	
#ifdef GLTF_SAVE
    std::optional<JsonError> Save(Document const& document, std::string documentFilePath, bool useBinaryFormat)
    {
        try
        {
            detail::ValidateBuffers(document.buffers, useBinaryFormat);

            detail::Save(document, documentFilePath, useBinaryFormat);
            return {};
        }
        catch (std::exception & e)
        {
			return JsonError{.file=documentFilePath,.what=e.what()};
        }		
    }
#endif
} // namespace gltf


} // namespace fx

#undef FX_GLTF_HAS_CPP_17
