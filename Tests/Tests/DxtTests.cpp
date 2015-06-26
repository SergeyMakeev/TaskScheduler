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
		double psnr = 10.0 * log10(255.0*255.0/mse) / log10(10.0);
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
	struct CompressDxtBlock : public MT::TaskBase<CompressDxtBlock>
	{
		DECLARE_DEBUG("CompressDxtBlock", MT_COLOR_BLUE);

		int srcX;
		int srcY;

		int stride;

		uint8 * srcPixels;
		uint8 * dstBlock;

		CompressDxtBlock(int _srcX, int _srcY, int _stride, uint8* _srcPixels, uint8* _dstBlock )
		{
				srcX = _srcX;
				srcY = _srcY;
				stride = _stride;
				srcPixels = _srcPixels;
				dstBlock = _dstBlock;
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

					MT_ASSERT(((size_t)(index + 2) < MT_ARRAY_SIZE(EmbeddedImage::lenaColor)), "Invalid index");

					uint8 r = srcPixels[index + 0];
					uint8 g = srcPixels[index + 1];
					uint8 b = srcPixels[index + 2];

					uint32 color = 0xFF000000 | ((b << 16) | (g << 8) | (r));

					pixels[y * 4 + x] = color;
				}
			}

			// compress the 4x4 block using DXT1 compression
			squish::Compress( (squish::u8 *)&pixels[0], dstBlock, squish::kDxt1 );
		}
	};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct CompressDxt : public MT::TaskBase<CompressDxt>
	{
		DECLARE_DEBUG("CompressDxt", MT_COLOR_DEFAULT);

		uint32 width;
		uint32 height;
		uint32 stride;
		uint8 * srcPixels;

		uint32 blkWidth;
		uint32 blkHeight;

		uint8 * dxtBlocks;

		CompressDxt(uint32 _width, uint32 _height, uint32 _stride, uint8* _srcPixels)
		{
			width = _width;
			height = _height;
			stride = _stride;

			srcPixels = _srcPixels;

			blkWidth = width >> 2;
			blkHeight = height >> 2;

			int dxtBlocksTotalSizeInBytes = blkWidth * blkHeight * 8; // 8 bytes = 64 bits per block (dxt1)
			dxtBlocks = (uint8 *)malloc( dxtBlocksTotalSizeInBytes );
			memset(dxtBlocks, 0x0, dxtBlocksTotalSizeInBytes);
		}

		~CompressDxt()
		{
			if (dxtBlocks)
			{
				free(dxtBlocks);
				dxtBlocks = nullptr;
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

					subTasks.PushBack( CompressDxtBlock(blkX * 4, blkY * 4, stride, srcPixels, &dxtBlocks[blockIndex * 8]) );
				}
			}

			context.RunSubtasksAndYield(MT::TaskGroup::GROUP_0, &subTasks[0], subTasks.Size());
		}
	};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct DecompressDxtBlock : public MT::TaskBase<DecompressDxtBlock>
	{
		DECLARE_DEBUG("DecompressDxtBlock", MT_COLOR_RED);

		int dstX;
		int dstY;

		int stride;

		uint8 * srcBlock;
		uint8 * dstPixels;

		DecompressDxtBlock(int _dstX, int _dstY, int _stride, uint8* _dstPixels, uint8* _srcBlock )
		{
			dstX = _dstX;
			dstY = _dstY;
			stride = _stride;
			dstPixels = _dstPixels;
			srcBlock = _srcBlock;
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
					squish::Decompress((squish::u8 *)&pixels[0], srcBlock, squish::kDxt1);

					int posX = dstX + x;
					int posY = dstY + y;

					int index = posY * stride + (posX * 3);

					uint32 pixel = pixels[y * 4 + x];

					MT_ASSERT(((size_t)(index + 2) < MT_ARRAY_SIZE(EmbeddedImage::lenaColor)), "Invalid index");

					dstPixels[index + 0] = (pixel & 0xFF);
					dstPixels[index + 1] = (pixel >> 8 & 0xFF);
					dstPixels[index + 2] = (pixel >> 16 & 0xFF);
				}
			}

		}
	};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct DecompressDxt : public MT::TaskBase<DecompressDxt>
	{
		DECLARE_DEBUG("DecompressDxt", MT_COLOR_YELLOW);

		uint8 * dxtBlocks;
		uint32 blkWidth;
		uint32 blkHeight;

		uint8 * decompressedImage;

		DecompressDxt(uint8 * _dxtBlocks, uint32 dxtBlocksCountWidth, uint32 dxtBlocksCountHeight)
		{
			dxtBlocks = _dxtBlocks;
			blkWidth = dxtBlocksCountWidth;
			blkHeight = dxtBlocksCountHeight;

			decompressedImage = (uint8*)malloc(dxtBlocksCountWidth*dxtBlocksCountHeight*48); // dxt1 block = 16 rgb pixels = 48 bytes
		}

		~DecompressDxt()
		{
			if (dxtBlocks)
			{
				free(dxtBlocks);
				dxtBlocks = nullptr;
			}

			if (decompressedImage)
			{
				free(decompressedImage);
				decompressedImage = nullptr;
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

					subTasks.PushBack( DecompressDxtBlock(blkX * 4, blkY * 4, stride, decompressedImage, &dxtBlocks[blockIndex * 8]) );
				}
			}

			context.RunSubtasksAndYield(MT::TaskGroup::GROUP_0, &subTasks[0], subTasks.Size());
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

#ifdef MT_INSTRUMENTED_BUILD
			scheduler.UpdateProfiler();
#endif
		}
	}


	// dxt compressor complex test
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	TEST(RunComplexDxtTest)
	{
		static_assert(MT_ARRAY_SIZE(EmbeddedImage::lenaColor) == 49152, "Image size is invalid");

		int stride = 384;

		uint8 * srcImage = (uint8 *)&EmbeddedImage::lenaColor[0];
		CompressDxt compressTask(128, 128, stride, srcImage);
		MT_ASSERT ((compressTask.width & 3) == 0 && (compressTask.height & 3) == 0, "Image size must be a multiple of 4");

		MT::TaskScheduler scheduler;
		int workerCount = (int)scheduler.GetWorkerCount();
		printf("Scheduler started, %d workers\n", workerCount);

#ifdef MT_INSTRUMENTED_BUILD
		printf("Profiler: http://127.0.0.1:%d\n", (int)scheduler.GetWebServerPort());
#endif

		scheduler.RunAsync(MT::TaskGroup::GROUP_0, &compressTask, 1);

		Wait(scheduler);

		DecompressDxt decompressTask(compressTask.dxtBlocks, compressTask.blkWidth, compressTask.blkHeight);
		compressTask.dxtBlocks = nullptr; //transfer ownership to Decompress task

		scheduler.RunAsync(MT::TaskGroup::GROUP_0, &decompressTask, 1);

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

		bool imagesEqual = CompareImagesPSNR(srcImage, decompressTask.decompressedImage, MT_ARRAY_SIZE(EmbeddedImage::lenaColor), 8.0);
		CHECK_EQUAL(true, imagesEqual);

#ifdef MT_INSTRUMENTED_BUILD
		// waiting for profiler attach
		printf("Press any key to continue\n");
		while(true)
		{
			scheduler.UpdateProfiler();
			if (_kbhit() != 0)
			{
				break;
			}
		}
#endif
	}


}
