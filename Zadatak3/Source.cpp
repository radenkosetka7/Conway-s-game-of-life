#pragma warning (disable : 4996)
#include <CL/cl.hpp>
#include <iostream>
#include <thread>
#include <chrono>


struct Pixel {
	unsigned char r, g, b;
};
char* readKernelSource(const char* filename)
{
	char* kernelSource = nullptr;
	long length;
	FILE* f = fopen(filename, "r");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		kernelSource = (char*)calloc(length, sizeof(char));
		if (kernelSource)
			fread(kernelSource, 1, length, f);
		fclose(f);
	}
	return kernelSource;

}
static void readImage(const char* filename, unsigned char*& array, int& width, int& height)
{
	FILE* fp = fopen(filename, "rb"); /* b - binary mode */
	if (!fscanf(fp, "P5\n%d %d\n255\n", &width, &height)) {
		throw "error";
	}
	unsigned char* image = new unsigned char[(size_t)width * height];
	fread(image, sizeof(unsigned char), (size_t)width * (size_t)height, fp);
	fclose(fp);
	array = image;
}
void writeImage(const char* filename, const unsigned char* array, const int width, const int height)
{
	FILE* fp = fopen(filename, "wb"); /* b - binary mode */
	fprintf(fp, "P5\n%d %d\n255\n", width, height);
	fwrite(array, sizeof(unsigned char), (size_t)width * (size_t)height, fp);
	fclose(fp);
}
void writeColoredImage(const char* filename, const Pixel* array, const int width, const int height)
{
	FILE* fp = fopen(filename, "wb"); /* b - binary mode */
	fprintf(fp, "P6\n%d %d\n255\n", width, height);
	fwrite(array, sizeof(Pixel), (size_t)width * (size_t)height, fp);
	fclose(fp);
}

void writeImageIteration(std::string filename)
{
	int width = -1;
	int height = -1;
	int numOfIterations;
	std::cout << "Unesite broj iteracija: ";
	std::cin >> numOfIterations;
	std::cout << '\n';
	unsigned char* image = nullptr;
	Pixel* coloredImage = nullptr;
	std::string option;
	std::cout << "Da li zelite kreirati sliku rucno? [0|1] ";
	std::cin >> option;
	std::cout << '\n';
	if (option == "0")
	{
		std::cout << "Unesite dimenzije slike: " << '\n';
		std::cout << "visina: ";
		std::cin >> height;
		std::cout << '\n';
		std::cout << "sirina: ";
		std::cin >> width;
		std::cout << '\n';
		image = (unsigned char*)calloc((width * height), sizeof(unsigned char));
		std::cout << "Unesite koordinate zivih celija: ";
		int x,y;
		std::string option;
		while (true)
		{
			std::cout << "x= ";
			std::cin >> x;
			std::cout << '\n';
			std::cout << "y= ";
			std::cin >> y;
			std::cout << '\n';
			if (!(x<0 || x>(width - 1)) || (y<0 || y>(height - 1)))
			{
				image[(y * width) + x] = 255;
			}
			std::cout << "Novi unos ? [y/n]" << '\n';
			std::cin >> option;
			if (option == "y")
				continue;
			else
				break;
		}
		writeImage(filename.c_str(), image, width, height);

	}
	else if (option == "1")
	{
		readImage(filename.c_str(), image, width, height);
	}
	else
	{
		throw "error";
	}
	size_t bytes1 = width * height * sizeof(Pixel);
	coloredImage = (Pixel*)malloc(bytes1);
	size_t globalSize[2], localSize[2];
	size_t bytes = width * height;
	localSize[0] = localSize[1] = 16;
	globalSize[0] = (size_t)ceil(width / (float)localSize[0]) * localSize[0];
	globalSize[1] = (size_t)ceil(height / (float)localSize[1]) * localSize[1];
	cl_platform_id cpPlatform;
	cl_device_id device_id;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;
	cl_int err;
	cl_mem oldPhoto;
	cl_mem newPhoto;
	cl_mem coloredPhoto;
	err = clGetPlatformIDs(1, &cpPlatform, NULL);
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	queue = clCreateCommandQueue(context, device_id, 0, &err);
	char* kernelSource = readKernelSource("OpenCL.cl");
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err);
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err)
	{
		size_t log_size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "createNewPhoto", &err);
	oldPhoto = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, NULL);
	newPhoto = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, NULL);
	coloredPhoto = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes1, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, oldPhoto, CL_TRUE, 0, bytes, image, 0, NULL, NULL);

	for (int i = 0; i < numOfIterations; i++)
	{
		err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &oldPhoto);
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &newPhoto);
		err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &coloredPhoto);
		err |= clSetKernelArg(kernel, 3, sizeof(int), &height);
		err |= clSetKernelArg(kernel, 4, sizeof(int), &width);
		clFinish(queue);
		err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalSize,localSize, 0, NULL, NULL);
		clFinish(queue);
		clEnqueueReadBuffer(queue, newPhoto, CL_TRUE, 0, bytes, image, 0, NULL, NULL); 
		clFinish(queue);
		clEnqueueReadBuffer(queue, coloredPhoto, CL_TRUE, 0, bytes1, coloredImage, 0, NULL, NULL);
		clFinish(queue);
		std::string photoName = "slike\\slika" + std::to_string(i + 1) + ".pgm";
		writeImage(photoName.c_str(),image, width, height);
		std::string photoName1 = "slike\\slika" + std::to_string(i) + "oscilator.ppm";
		writeColoredImage(photoName1.c_str(), coloredImage, width, height);
		cl_mem helped = oldPhoto;
		oldPhoto = newPhoto;
		newPhoto = helped;
	}
	clReleaseMemObject(oldPhoto);
	clReleaseMemObject(newPhoto);
	clReleaseMemObject(coloredPhoto);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	free(image);
	free(coloredImage);
}

void moveToArbitraryIteration(std::string filename)
{
	int iteration;
	std::cout << "Unesite broj iteracije na koju zelite preci: ";
	std::cin >> iteration;
	std::cout << '\n';
	int width = -1, height = -1;
	unsigned char* image = nullptr;
	Pixel* coloredImage = nullptr;
	readImage(filename.c_str(), image, width, height);
	size_t globalSize[2], localSize[2];
	size_t bytes = width * height;
	size_t bytes1 = width * height * sizeof(Pixel);
	coloredImage = (Pixel*)malloc(bytes1);
	localSize[0] = localSize[1] = 16;
	globalSize[0] = (size_t)ceil(width / (float)localSize[0]) * localSize[0];
	globalSize[1] = (size_t)ceil(height / (float)localSize[1]) * localSize[1];
	cl_platform_id cpPlatform;
	cl_device_id device_id;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;
	cl_int err;
	cl_mem oldPhoto;
	cl_mem newPhoto;
	cl_mem coloredPhoto;
	err = clGetPlatformIDs(1, &cpPlatform, NULL);
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	queue = clCreateCommandQueue(context, device_id, 0, &err);
	char* kernelSource = readKernelSource("OpenCL.cl");
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err);
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err)
	{
		size_t log_size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "createNewPhoto", &err);
	oldPhoto = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, NULL);
	newPhoto = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, NULL);
	coloredPhoto = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes1, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, oldPhoto, CL_TRUE, 0, bytes, image, 0, NULL, NULL);
	for (int i = 0; i < iteration; i++)
	{
		err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &oldPhoto);
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &newPhoto);
		err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &coloredPhoto);
		err |= clSetKernelArg(kernel, 3, sizeof(int), &height);
		err |= clSetKernelArg(kernel, 4, sizeof(int), &width);
		clFinish(queue);
		err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalSize, localSize, 0, NULL, NULL);
		clFinish(queue);
		clEnqueueReadBuffer(queue, newPhoto, CL_TRUE, 0, bytes, image, 0, NULL, NULL);
		clFinish(queue);
		clEnqueueReadBuffer(queue, coloredPhoto, CL_TRUE, 0, bytes1, coloredImage, 0, NULL, NULL);
		clFinish(queue);
		cl_mem helped = oldPhoto;
		oldPhoto = newPhoto;
		newPhoto = helped;
	}
	std::string photoName = "slike\\rezultat" + std::to_string(iteration) + ".iteracije" + ".pgm";
	writeImage(filename.c_str(), image, width, height);
	clReleaseMemObject(oldPhoto);
	clReleaseMemObject(newPhoto);
	clReleaseMemObject(coloredPhoto);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	free(image);
	free(coloredImage);
}

void setSubsegment(std::string filename)
{
	int width = -1, height = -1, segmentWidth = -1, segmentHeight = -1;
	unsigned char* image = nullptr;
	unsigned char* segmentImage = nullptr;
	readImage(filename.c_str(), segmentImage, segmentWidth, segmentHeight);
	do
	{
		std::cout << "Unesite dimenzije slike na koju zelite staviti segment originalne slike:" << '\n';
		std::cout << "sirina = ";
		std::cin >> width;
		std::cout << '\n' << "visina= ";
		std::cin >> height;
		std::cout << '\n';
	} while (width < 0 || height < 0 || width < segmentWidth || height < segmentHeight);
	int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
	std::cout << "Unesite koordinate na kojima zelite da se nalazi segment na slici: ";
	do
	{
		std::cout << "x1= ";
		std::cin >> x1;
		std::cout << '\n';
		std::cout << "y1= ";
		std::cin >> y1;
		std::cout << '\n';
		std::cout << "x2= ";
		std::cin >> x2;
		std::cout << '\n';
		std::cout << "y2= ";
		std::cin >> y2;
		std::cout << '\n';
	} while (x1<0 || y1<0 || x2 < 0 || y2 < 0 || x1>(width) || y1>(height) || x2>(width) || y2>(height) || x2 <= x1 || y2 <= y1 || ((x2 - x1) != segmentWidth) || ((y2 - y1) != segmentHeight));
	image = (unsigned char*)calloc(width * height, sizeof(unsigned char));
	size_t globalSize[2], localSize[2];
	size_t imageSize = width * height;
	size_t sizeOfSegment = segmentWidth * segmentHeight;
	localSize[0] = localSize[1] = 16;
	globalSize[0] = (size_t)ceil(width / (float)localSize[0]) * localSize[0];
	globalSize[1] = (size_t)ceil(height / (float)localSize[1]) * localSize[1];
	cl_platform_id cpPlatform;
	cl_device_id device_id;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;
	cl_int err;
	cl_mem oldPhoto;
	cl_mem segmentPhoto;
	err = clGetPlatformIDs(1, &cpPlatform, NULL);
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	queue = clCreateCommandQueue(context, device_id, 0, &err);
	char* kernelSource = readKernelSource("OpenCL.cl");
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err);
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err)
	{
		size_t log_size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "setSubsegment", &err);
	oldPhoto = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeOfSegment, NULL, NULL);
	segmentPhoto = clCreateBuffer(context, CL_MEM_WRITE_ONLY, imageSize, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, oldPhoto, CL_TRUE, 0, sizeOfSegment, segmentImage, 0, NULL, NULL);
	clFinish(queue);
	err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &oldPhoto);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &segmentPhoto);
	err |= clSetKernelArg(kernel, 2, sizeof(int), &width);
	err |= clSetKernelArg(kernel, 3, sizeof(int), &segmentWidth);
	err |= clSetKernelArg(kernel, 4, sizeof(int), &segmentHeight);
	err |= clSetKernelArg(kernel, 5, sizeof(int), &x1);
	err |= clSetKernelArg(kernel, 6, sizeof(int), &y1);
	err |= clSetKernelArg(kernel, 7, sizeof(int), &x2);
	err |= clSetKernelArg(kernel, 8, sizeof(int), &y2);
	clFinish(queue);
	err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalSize, localSize, 0, NULL, NULL);
	clFinish(queue);
	clEnqueueReadBuffer(queue, segmentPhoto, CL_TRUE, 0, imageSize, image, 0, NULL, NULL);
	clFinish(queue);
	std::string name = filename.substr(0, filename.find("."));
	std::string segmentfile = name + "podsegment1.pgm";
	writeImage(segmentfile.c_str(), image, width, height);
	clReleaseMemObject(oldPhoto);
	clReleaseMemObject(segmentPhoto);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	free(image);
	free(segmentImage);
}
void getSubsegment(std::string filename)
{
	int width = -1, height = -1;
	unsigned char* image = nullptr;
	unsigned char* segmentImage = nullptr;
	readImage(filename.c_str(), image, width, height);
	int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
	std::cout << "Unesite koordinate podsegmenta: ";
	do
	{
		std::cout << "x1= "; 
		std::cin >> x1;
		std::cout <<'\n';
		std::cout << "y1= "; 
		std::cin >> y1; 
		std::cout <<'\n';
		std::cout << "x2= ";
		std::cin >> x2;
		std::cout <<'\n';
		std::cout << "y2= "; 
		std::cin >> y2;
		std::cout << '\n';
	} while (x1<0 || y1<0 || x2 < 0 || y2 < 0 || x1>(width) || y1>(height) || x2>(width) || y2>(height) || x2 <= x1 || y2 <= y1);
	size_t widthSegment = x2 - x1 + 1;
	size_t heightSegment = y2 - y1 + 1;
	int widthOfSegment = x2 - x1;
	size_t sizeOfSegment = widthSegment * heightSegment;
	segmentImage = (unsigned char*)calloc(sizeOfSegment, sizeof(unsigned char));
	size_t globalSize[2], localSize[2];
	size_t imageSize = width * height;
	localSize[0] = localSize[1] = 16;
	globalSize[0] = (size_t)ceil(width / (float)localSize[0]) * localSize[0];
	globalSize[1] = (size_t)ceil(height / (float)localSize[1]) * localSize[1];
	cl_platform_id cpPlatform;
	cl_device_id device_id;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;
	cl_int err;
	cl_mem oldPhoto;
	cl_mem segmentPhoto;
	err = clGetPlatformIDs(1, &cpPlatform, NULL);
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	queue = clCreateCommandQueue(context, device_id, 0, &err);
	char* kernelSource = readKernelSource("OpenCL.cl");
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err);
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err)
	{
		size_t log_size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "getSubsegment", &err);
	oldPhoto = clCreateBuffer(context, CL_MEM_READ_ONLY, imageSize, NULL, NULL);
	segmentPhoto = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeOfSegment, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, oldPhoto, CL_TRUE, 0, imageSize, image, 0, NULL, NULL);
	clFinish(queue);
	err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &oldPhoto);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &segmentPhoto);
	err |= clSetKernelArg(kernel, 2, sizeof(int), &widthOfSegment);
	err |= clSetKernelArg(kernel, 3, sizeof(int), &width);
	err |= clSetKernelArg(kernel, 4, sizeof(int), &height);
	err |= clSetKernelArg(kernel, 5, sizeof(int), &x1);
	err |= clSetKernelArg(kernel, 6, sizeof(int), &y1);
	err |= clSetKernelArg(kernel, 7, sizeof(int), &x2);
	err |= clSetKernelArg(kernel, 8, sizeof(int), &y2);
	clFinish(queue);
	err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalSize, localSize, 0, NULL, NULL);
	clFinish(queue);
	clEnqueueReadBuffer(queue, segmentPhoto, CL_TRUE, 0, sizeOfSegment, segmentImage, 0, NULL, NULL);
	clFinish(queue);
	std::string name = filename.substr(0, filename.find("."));
	std::string segmentfile = name + "podsegment.pgm";
	writeImage(segmentfile.c_str(),segmentImage, widthSegment-1, heightSegment-1);
	clReleaseMemObject(oldPhoto);
	clReleaseMemObject(segmentPhoto);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	free(image);
	free(segmentImage);
	setSubsegment(segmentfile);
}


int main()
{
	//writeImageIteration("slike\\slika0.pgm");
	//moveToArbitraryIteration("slike\\slika0.pgm");
	getSubsegment("slike\\slika0.pgm");
	system("PAUSE");
}
