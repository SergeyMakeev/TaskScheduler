// The MIT License (MIT)
//
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
//
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>

#include <squish.h>
#include <string.h>

#ifdef _WIN32

#include <conio.h>

#else

#include <math.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int _kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}
#endif


#ifdef MT_INSTRUMENTED_BUILD

class Microprofile : public MT::IProfilerEventListener
{
	void OnThreadCreated(uint32 workerIndex)
	{
		workerIndex;
	}

	void OnThreadStarted(uint32 workerIndex)
	{
		workerIndex;
	}

	void OnThreadStoped(uint32 workerIndex)
	{
		workerIndex;
	}

	void OnThreadIdleBegin(uint32 workerIndex)
	{
		workerIndex;
	}

	void OnThreadIdleEnd(uint32 workerIndex)
	{
		workerIndex;
	}

	void OnTaskFinished(MT::Color::Type debugColor, const mt_char* debugID)
	{
		debugColor;
		debugID;
	}

	void OnTaskResumed(MT::Color::Type debugColor, const mt_char* debugID)
	{
		debugColor;
		debugID;
	}

	void OnTaskYielded(MT::Color::Type debugColor, const mt_char* debugID)
	{
		debugColor;
		debugID;
	}

};


#endif





namespace EmbeddedImage
{
	#include "LenaDxt/LenaColor.h"
	#include "LenaDxt/HeaderDDS.h"
}


bool CompareImagesPSNR(uint8 * img1, uint8 * img2, uint32 bytesCount, double psnrThreshold)
{
	double mse = 0.0;

	for (uint32 i = 0; i < bytesCount; i++)
	{
		double error = (double)img1[0] - (double)img2[1];
		mse += (error * error);
	}

	mse = mse / (double)bytesCount;

	if (mse > 0.0)
	{
		double psnr = 10.0 * log10(255.0*255.0/mse);
		if (psnr < psnrThreshold)
		{
			return false;
		}
	}

	return true;
}



SUITE(DxtTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct CompressDxtBlock
	{
		MT_DECLARE_TASK(CompressDxtBlock, MT::StackRequirements::STANDARD, MT::Color::Blue);

		MT::ArrayView<uint8> srcPixels;
		MT::ArrayView<uint8> dstBlocks;

		int srcX;
		int srcY;

		int stride;
		int dstBlockOffset;

		CompressDxtBlock(int _srcX, int _srcY, int _stride, const MT::ArrayView<uint8> & _srcPixels, const MT::ArrayView<uint8> & _dstBlocks, int _dstBlockOffset)
			: srcPixels(_srcPixels)
			, dstBlocks(_dstBlocks)
		{
				srcX = _srcX;
				srcY = _srcY;
				stride = _stride;
				dstBlockOffset = _dstBlockOffset;
		}

		CompressDxtBlock(CompressDxtBlock&& other)
			: srcPixels(other.srcPixels)
			, dstBlocks(other.dstBlocks)
			, srcX(other.srcX)
			, srcY(other.srcY)
			, stride(other.stride)
			, dstBlockOffset(other.dstBlockOffset)
		{
			other.srcX = -1;
			other.srcY = -1;
			other.stride = -1;
			other.dstBlockOffset = -1;
		}

		~CompressDxtBlock()
		{
			srcX = -1;
			srcY = -1;
			stride = -1;
			dstBlockOffset = -1;
		}

		void Do(MT::FiberContext&)
		{
			// 16 pixels of input
			uint32 pixels[4*4];

			// copy dxt1 block from image
			for (int y = 0; y < 4; y++)
			{
				for (int x = 0; x < 4; x++)
				{
					int posX = srcX + x;
					int posY = srcY + y;

					int index = posY * stride + (posX * 3);

					MT_ASSERT(index >= 0 && ((size_t)(index + 2) < MT_ARRAY_SIZE(EmbeddedImage::lenaColor)), "Invalid index");

					uint8 r = srcPixels[index + 0];
					uint8 g = srcPixels[index + 1];
					uint8 b = srcPixels[index + 2];

					uint32 color = 0xFF000000 | ((b << 16) | (g << 8) | (r));

					pixels[y * 4 + x] = color;
				}
			}

			// compress the 4x4 block using DXT1 compression
			squish::Compress( (squish::u8 *)&pixels[0], &dstBlocks[dstBlockOffset], squish::kDxt1 );
		}
	};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct CompressDxt
	{
		MT_DECLARE_TASK(CompressDxt, MT::StackRequirements::EXTENDED, MT::Color::Aqua);

		uint32 width;
		uint32 height;
		uint32 stride;

		uint32 blkWidth;
		uint32 blkHeight;

		MT::ArrayView<uint8> srcPixels;
		MT::ArrayView<uint8> dxtBlocks;

		CompressDxt(uint32 _width, uint32 _height, uint32 _stride, const MT::ArrayView<uint8> & _srcPixels )
			: srcPixels(_srcPixels)
		{
			width = _width;
			height = _height;
			stride = _stride;

			blkWidth = width >> 2;
			blkHeight = height >> 2;

			int dxtBlocksTotalSizeInBytes = blkWidth * blkHeight * 8; // 8 bytes = 64 bits per block (dxt1)
			dxtBlocks = MT::ArrayView<uint8>( malloc( dxtBlocksTotalSizeInBytes ), dxtBlocksTotalSizeInBytes);
		}

		~CompressDxt()
		{
			void* pDxtBlocks = dxtBlocks.GetRawData();
			if (pDxtBlocks)
			{
				free(pDxtBlocks);
			}
		}


		void Do(MT::FiberContext& context)
		{
			// use stack_array as subtask container. beware stack overflow!
			MT::StackArray<CompressDxtBlock, 1024> subTasks;

			for (uint32 blkY = 0; blkY < blkHeight; blkY++)
			{
				for (uint32 blkX = 0; blkX < blkWidth; blkX++)
				{
					uint32 blockIndex = blkY * blkWidth + blkX;
					subTasks.PushBack( CompressDxtBlock(blkX * 4, blkY * 4, stride, srcPixels, dxtBlocks, blockIndex * 8) );
				}
			}

			context.RunSubtasksAndYield(MT::TaskGroup::Default(), &subTasks[0], subTasks.Size());
		}
	};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct DecompressDxtBlock
	{
		MT_DECLARE_TASK(DecompressDxtBlock, MT::StackRequirements::STANDARD, MT::Color::Red);

		MT::ArrayView<uint8> srcBlocks;
		MT::ArrayView<uint8> dstPixels;

		int dstX;
		int dstY;

		int stride;
		int srcBlockOffset;

		DecompressDxtBlock(int _dstX, int _dstY, int _stride, const MT::ArrayView<uint8> & _dstPixels, const MT::ArrayView<uint8> & _srcBlocks, int _srcBlockOffset)
			: srcBlocks(_srcBlocks)
			, dstPixels(_dstPixels)
		{
			dstX = _dstX;
			dstY = _dstY;
			stride = _stride;
			srcBlockOffset = _srcBlockOffset;
		}

		DecompressDxtBlock(DecompressDxtBlock&& other)
			: srcBlocks(other.srcBlocks)
			, dstPixels(other.dstPixels)
			, dstX(other.dstX)
			, dstY(other.dstY)
			, stride(other.stride)
			, srcBlockOffset(other.srcBlockOffset)
		{
			other.dstX = -1;
			other.dstY = -1;
			other.stride = -1;
			other.srcBlockOffset = -1;
		}

		~DecompressDxtBlock()
		{
			dstX = -1;
			dstY = -1;
			stride = -1;
			srcBlockOffset = -1;
		}


		void Do(MT::FiberContext&)
		{
			// 16 pixels of output
			uint32 pixels[4*4];

			// copy dxt1 block from image
			for (int y = 0; y < 4; y++)
			{
				for (int x = 0; x < 4; x++)
				{
					squish::Decompress((squish::u8 *)&pixels[0], &srcBlocks[srcBlockOffset], squish::kDxt1);

					int posX = dstX + x;
					int posY = dstY + y;

					int index = posY * stride + (posX * 3);

					uint32 pixel = pixels[y * 4 + x];

					MT_ASSERT(index >= 0 && ((size_t)(index + 2) < MT_ARRAY_SIZE(EmbeddedImage::lenaColor)), "Invalid index");

					dstPixels[index + 0] = (pixel & 0xFF);
					dstPixels[index + 1] = (pixel >> 8 & 0xFF);
					dstPixels[index + 2] = (pixel >> 16 & 0xFF);
				}
			}

		}
	};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct DecompressDxt
	{
		MT_DECLARE_TASK(DecompressDxt, MT::StackRequirements::EXTENDED, MT::Color::Yellow);

		MT::ArrayView<uint8> dxtBlocks;
		MT::ArrayView<uint8> decompressedImage;

		uint32 blkWidth;
		uint32 blkHeight;


		DecompressDxt(const MT::ArrayView<uint8> & _dxtBlocks, uint32 dxtBlocksCountWidth, uint32 dxtBlocksCountHeight)
			: dxtBlocks(_dxtBlocks)
		{
			blkWidth = dxtBlocksCountWidth;
			blkHeight = dxtBlocksCountHeight;

			// dxt1 block = 16 rgb pixels = 48 bytes
			uint32 bytesCount = blkWidth * blkHeight * 48;
			decompressedImage = MT::ArrayView<uint8>( malloc(bytesCount), bytesCount);
		}

		~DecompressDxt()
		{
			void* pDxtBlocks = dxtBlocks.GetRawData();
			if (pDxtBlocks)
			{
				free(pDxtBlocks);
			}

			void* pDecompressedImage = decompressedImage.GetRawData();
			if (pDecompressedImage)
			{
				free(pDecompressedImage);
			}

		}

		void Do(MT::FiberContext& context)
		{
			// use stack_array as subtask container. beware stack overflow!
			MT::StackArray<DecompressDxtBlock, 1024> subTasks;

			int stride = blkWidth * 4 * 3;

			for (uint32 blkY = 0; blkY < blkHeight; blkY++)
			{
				for (uint32 blkX = 0; blkX < blkWidth; blkX++)
				{
					uint32 blockIndex = blkY * blkWidth + blkX;
					subTasks.PushBack( DecompressDxtBlock(blkX * 4, blkY * 4, stride, decompressedImage, dxtBlocks, blockIndex * 8) );
				}
			}

			context.RunSubtasksAndYield(MT::TaskGroup::Default(), &subTasks[0], subTasks.Size());
		}

	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Wait(MT::TaskScheduler & scheduler)
	{
		//emulate game loop
		while(true)
		{
			bool waitDone = scheduler.WaitAll(33);
			if (waitDone)
			{
				break;
			}
		}
	}


	// dxt compressor complex test
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	TEST(RunComplexDxtTest)
	{
		static_assert(MT_ARRAY_SIZE(EmbeddedImage::lenaColor) == 49152, "Image size is invalid");

		int stride = 384;

		MT::ArrayView<uint8> srcImage((void*)&EmbeddedImage::lenaColor[0], MT_ARRAY_SIZE(EmbeddedImage::lenaColor));

		CompressDxt compressTask(128, 128, stride, srcImage);
		MT_ASSERT ((compressTask.width & 3) == 0 && (compressTask.height & 3) == 0, "Image size must be a multiple of 4");

#ifdef MT_INSTRUMENTED_BUILD
		Microprofile profiler;
		MT::TaskScheduler scheduler(0, &profiler);
#else
		MT::TaskScheduler scheduler;
#endif

		int workersCount = (int)scheduler.GetWorkersCount();
		printf("Scheduler started, %d workers\n", workersCount);

		printf("Compress image\n");
		scheduler.RunAsync(MT::TaskGroup::Default(), &compressTask, 1);

		Wait(scheduler);

		DecompressDxt decompressTask(compressTask.dxtBlocks, compressTask.blkWidth, compressTask.blkHeight);
		compressTask.dxtBlocks = MT::ArrayView<uint8>(); //transfer memory ownership to Decompress task

		printf("Decompress image\n");
		scheduler.RunAsync(MT::TaskGroup::Default(), &decompressTask, 1);

		Wait(scheduler);

/*
		//save compressed image
		{
			FILE * file = fopen("lena_dxt1.dds", "w+b");
			fwrite(&EmbeddedImage::ddsHeader[0], MT_ARRAY_SIZE(EmbeddedImage::ddsHeader), 1, file);
			fwrite(decompressTask.dxtBlocks, decompressTask.blkWidth * decompressTask.blkHeight * 8, 1, file);
			fclose(file);
		}

		//save uncompressed image
		{
			FILE * file = fopen("lena_rgb.raw", "w+b");
			fwrite(decompressTask.decompressedImage, decompressTask.blkWidth * decompressTask.blkHeight * 48, 1, file);
			fclose(file);
		}
*/

		printf("Compare images\n");
		bool imagesEqual = CompareImagesPSNR(&srcImage[0], &decompressTask.decompressedImage[0], MT_ARRAY_SIZE(EmbeddedImage::lenaColor), 8.0);
		CHECK_EQUAL(true, imagesEqual);

#ifdef MT_INSTRUMENTED_BUILD
		// waiting for profiler attach
		printf("Press any key to continue\n");
		while(true)
		{
			if (_kbhit() != 0)
			{
				break;
			}
		}
#endif
	}


}
