#include "WICImageLoader.h"

#include <wincodec.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>

#pragma comment(lib, "windowscodecs.lib")

using namespace Microsoft::WRL;

namespace WICImageLoader
{


	///@todo returning ImageData seem to be fine, mordern compilers use move semantics instead of copy. Need to verify this.
	ImageData LoadImageFromMemory_WIC(const uint8_t* data, size_t sizeInBytes)
	{
		ImageData outputResource;

		//Create WIC factory
		ComPtr<IWICImagingFactory> factory;
		HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));

		//Initialize WIC Stream from memory
		ComPtr<IWICStream> stream;
		factory->CreateStream(&stream);
		stream->InitializeFromMemory(const_cast<BYTE*>(data), static_cast<DWORD>(sizeInBytes));

		//Decode Image
		ComPtr<IWICBitmapDecoder> decoder;
		factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder);

		ComPtr<IWICBitmapFrameDecode> frame;
		decoder->GetFrame(0, &frame);


		//Output: Width and Height
		frame->GetSize(&outputResource.width, &outputResource.height);

		// Convert to RGBA8
		ComPtr<IWICFormatConverter> converter;
		factory->CreateFormatConverter(&converter);

		converter->Initialize(frame.Get(),
			GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone,
			nullptr, 0.0f, WICBitmapPaletteTypeCustom);

		const uint32_t bytesPerPixel = 4;
		const uint32_t rowPitch = outputResource.width * bytesPerPixel;
		const uint32_t imageSize = rowPitch * outputResource.height;

		outputResource.pixels.resize(imageSize);

		converter->CopyPixels(nullptr, rowPitch, imageSize, outputResource.pixels.data());

		return outputResource;
	}
}
