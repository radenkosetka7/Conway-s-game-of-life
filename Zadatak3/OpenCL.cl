struct Pixel
{
	unsigned char r,g,b;
};

bool isBlinkerVertical(__global unsigned char* oldPhoto, int x, int y, const int width)
{
	if(oldPhoto[(y*width)+x] == 255
		&& oldPhoto[((y-1)*width)+x] == 255
		&& oldPhoto[((y+1)*width)+x] == 255
		&& oldPhoto[((y-2)*width)+x] == 0
		&& oldPhoto[((y+2)*width)+x] == 0
		&& oldPhoto[((y-2)*width)+(x-1)] == 0
		&& oldPhoto[((y-2)*width)+(x+1)] == 0
		&& oldPhoto[((y-1)*width)+(x-1)] == 0
		&& oldPhoto[((y-1)*width)+(x+1)] == 0
		&& oldPhoto[(y*width)+(x-1)] == 0
		&& oldPhoto[(y*width)+(x+1)] == 0
		&& oldPhoto[((y+1)*width)+(x-1)] == 0
		&& oldPhoto[((y+1)*width)+(x+1)] == 0
		&& oldPhoto[((y+2)*width)+(x-1)] == 0
		&& oldPhoto[((y+2)*width)+(x+1)] == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
}

bool isBlinkerHorizontal(__global unsigned char* oldPhoto, int x, int y, const int width)
{
	if(oldPhoto[(y*width)+x] == 255
		&& oldPhoto[(y*width)+(x-1)] == 255
		&& oldPhoto[(y*width)+(x+1)] == 255
		&& oldPhoto[(y*width)+(x-2)] == 0
		&& oldPhoto[(y*width)+(x+2)] == 0
		&& oldPhoto[((y-1)*width)+(x-2)] == 0
		&& oldPhoto[((y-1)*width)+(x-1)] == 0
		&& oldPhoto[((y-1)*width)+x] == 0
		&& oldPhoto[((y-1)*width)+(x+1)] == 0
		&& oldPhoto[((y-1)*width)+(x+2)] == 0
		&& oldPhoto[((y+1)*width)+(x-2)] == 0
		&& oldPhoto[((y+1)*width)+(x-1)] == 0
		&& oldPhoto[((y+1)*width)+x] == 0
		&& oldPhoto[((y+1)*width)+(x+1)] == 0
		&& oldPhoto[((y+1)*width)+(x+2)] == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
}

__kernel void createNewPhoto(__global unsigned char* oldPhoto, __global unsigned char* newPhoto,
								__global struct Pixel* coloredPhoto,
								const int height, const int width)

{
	int x=get_global_id(0);
	int y=get_global_id(1);

	if(x==0 || y==0 || x>=width-1 || y>=height-1)
	{
		return;
	}
	
	int liveNeighbours=0;
	if(oldPhoto[((y-1)*width)+x] == 255)
		liveNeighbours++;
	if(oldPhoto[((y+1)*width)+x] == 255)
		liveNeighbours++;
	if(oldPhoto[(y*width)+(x-1)] == 255)
		liveNeighbours++;
	if(oldPhoto[((y+1)*width)+(x-1)] == 255)
		liveNeighbours++;
	if(oldPhoto[((y-1)*width)+(x-1)] == 255)
		liveNeighbours++;
	if(oldPhoto[((y-1)*width)+(x+1)] == 255)
		liveNeighbours++;
	if(oldPhoto[((y+1)*width)+(x-1)] == 255)
		liveNeighbours++;
	if(oldPhoto[((y+1)*width)+(x+1)] == 255)
		liveNeighbours++;
	if(oldPhoto[(y*width)+x]==255 && (liveNeighbours<2 || liveNeighbours>3))
		newPhoto[(y*width)+x]=0;
	else if(oldPhoto[(y*width)+x]==255 && liveNeighbours>=2)
		newPhoto[(y*width)+x]=255;
	else if(oldPhoto[(y*width)+x]==0 && liveNeighbours==3)
		newPhoto[(y*width)+x]=255;
	else
		newPhoto[(y*width)+x]=0;

	if (oldPhoto[(y * width) + x] == 255)
	{
		coloredPhoto[(y * width) + x].r = 255;
		coloredPhoto[(y * width) + x].g = 255;
		coloredPhoto[(y * width) + x].b = 255;
	}
	else
	{
		coloredPhoto[(y * width) + x].r = 0;
		coloredPhoto[(y * width) + x].g = 0;
		coloredPhoto[(y * width) + x].b = 0;
	}
	if(((x>=1 && y>=2) && (x<=(width-2) && y<=(height-3))) && isBlinkerVertical(oldPhoto,x,y,width))
	{
		coloredPhoto[(y*width)+x].r=0;
		coloredPhoto[(y*width)+x].g=0;
		coloredPhoto[(y*width)+x].b=255;
		coloredPhoto[((y-1)*width)+x].r=0;
		coloredPhoto[((y-1)*width)+x].g=0;
		coloredPhoto[((y-1)*width)+x].b=255;
		coloredPhoto[((y+1)*width)+x].r=0;
		coloredPhoto[((y+1)*width)+x].g=0;
		coloredPhoto[((y+1)*width)+x].b=255;
	}
	else if(((x>=2 && y>=1) && (x<=(width-3) && y<=(height-2))) && isBlinkerHorizontal(oldPhoto,x,y,width))
	{
		coloredPhoto[(y*width)+x].r=0;
		coloredPhoto[(y*width)+x].g=0;
		coloredPhoto[(y*width)+x].b=255;
		coloredPhoto[(y*width)+(x-1)].r=0;
		coloredPhoto[(y*width)+(x-1)].g=0;
		coloredPhoto[(y*width)+(x-1)].b=255;
		coloredPhoto[(y*width)+(x+1)].r=0;
		coloredPhoto[(y*width)+(x+1)].g=0;
		coloredPhoto[(y*width)+(x+1)].b=255;
	}

}

__kernel void getSubsegment(__global unsigned char* oldPhoto, __global unsigned char* newPhoto,const int widthOfSegment,
	const int oldWidth, const int oldHeight,const int x1,const int y1,const int x2,const int y2)
{
		int x = get_global_id(0);
		int y = get_global_id(1);
		if (!(x < x1 || x > x2 || y < y1 || y > y2))
		{
			int xt = x - x1;
			int yt = y - y1;
			newPhoto[(yt*widthOfSegment) + xt] = oldPhoto[(y * oldWidth) + x];
		}
}


__kernel void setSubsegment(__global unsigned char* oldPhoto, __global unsigned char* newPhoto,const int widthOfSegment,
	const int oldWidth, const int oldHeight,const int x1,const int y1,const int x2,const int y2)
{
		int x = get_global_id(0);
		int y = get_global_id(1);
		if (!(x < 0 || x > oldWidth || y < 0 || y > oldHeight))
		{
			int xt = x1 + x;
			int yt = y1 + y;
			newPhoto[yt*widthOfSegment + xt] = oldPhoto[(y * oldWidth) + x];
		}
}
