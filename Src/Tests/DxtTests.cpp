#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"

#include <Squish.h>

namespace EmbeddedImage
{
	#include "LenaColor.h"
	#include "LenaColorDXT1.h"
}


SUITE(DxtTests)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace DxtCompress
{
	struct TaskParams
	{
		uint32 width;
		uint32 height;
		uint32 stride;
		uint8 * srcPixels;

		uint32 blkWidth;
		uint32 blkHeight;

		uint8 * dstBlocks;
	};


	void MT_CALL_CONV Run(MT::FiberContext&, void* userData)
	{
		const TaskParams & params = *(TaskParams*)userData;

		for (uint32 blkY = 0; blkY < params.blkHeight; blkY++)
		{
			for (uint32 blkX = 0; blkX < params.blkWidth; blkX++)
			{
				// 16 pixels of input
				uint32 pixels[4*4];

				// copy dxt1 block from image
				for (int y = 0; y < 4; y++)
				{
					for (int x = 0; x < 4; x++)
					{
						int srcX = blkX * 4 + x;
						int srcY = blkY * 4 + y;

						int index = srcY * params.stride + (srcX * 3);

						uint8 r = params.srcPixels[index + 0];
						uint8 g = params.srcPixels[index + 1];
						uint8 b = params.srcPixels[index + 2];

						uint32 color = 0xFF000000 | ((b << 16) | (g << 8) | (r));

						pixels[y * 4 + x] = color;
					}
				}

				// 8 bytes of output
				int blockIndex = blkY * params.blkWidth + blkX;
				uint8 * dxtBlock = &params.dstBlocks[blockIndex*8];

				// compress the 4x4 block using DXT1 compression
				squish::Compress( (squish::u8 *)&pixels[0], dxtBlock, squish::kDxt1 );

			} // block iterator
		} // block iterator
	}


	// dxt compressor simple test
	TEST(RunSimpleDxtCompress)
	{
		static_assert(ARRAY_SIZE(EmbeddedImage::lenaColor) == 49152, "Image size is invalid");
		static_assert(ARRAY_SIZE(EmbeddedImage::lenaColorDXT1) == 8192, "Image size is invalid");

		TaskParams taskParams;
		taskParams.width = 128;
		taskParams.height = 128;
		taskParams.stride = 384;
		taskParams.srcPixels = (uint8 *)&EmbeddedImage::lenaColor[0];

		ASSERT ((taskParams.width & 3) == 0 && (taskParams.height & 4) == 0, "Image size must be a multiple of 4");

		taskParams.blkWidth = taskParams.width >> 2;
		taskParams.blkHeight = taskParams.height >> 2;

		int dxtBlocksTotalSize = taskParams.blkWidth * taskParams.blkHeight * 8;
		taskParams.dstBlocks = (uint8 *)malloc( dxtBlocksTotalSize );
		memset(taskParams.dstBlocks, 0x0, dxtBlocksTotalSize);

		MT::TaskScheduler scheduler;
		MT::TaskDesc task(DxtCompress::Run, &taskParams);

		scheduler.RunTasks(MT::TaskGroup::GROUP_0, &task, 1);

		CHECK(scheduler.WaitAll(30000));
		CHECK_ARRAY_EQUAL(taskParams.dstBlocks, EmbeddedImage::lenaColorDXT1, dxtBlocksTotalSize);

/*
		FILE * file = fopen("lena_dxt1.dds", "w+b");
		fwrite(&EmbeddedImage::ddsHeader[0], ARRAY_SIZE(EmbeddedImage::ddsHeader), 1, file);
		fwrite(taskParams.dstBlocks, dxtBlocksTotalSize, 1, file);
		fclose(file);
*/

		free(taskParams.dstBlocks);
	}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
	// dxt compressor complex test
	TEST(RunComplexDxtCompress)
	{
	}
*/

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}