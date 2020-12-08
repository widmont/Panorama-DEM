#include "MainPano.h"
#include "bitmap_image.hpp"

using namespace Concurrency;
bool testPix(unsigned int x, unsigned int y, unsigned char ridgecolorR, unsigned char ridgecolorG, unsigned char ridgecolorB, bitmap_image image)
{
	if (y >= image_height)
		return false;
	unsigned char r;
	unsigned char g;
	unsigned char b;

	image.get_pixel(x, y, r, g, b);
	return (r != ridgecolorR || g != ridgecolorG || b != ridgecolorB);
}

bool testPix(unsigned char r, unsigned char g, unsigned char b, unsigned char ridgecolorR, unsigned char ridgecolorG, unsigned char ridgecolorB, int y)
{
	if (y >= image_height)
		return false;
	return (r != ridgecolorR || g != ridgecolorG || b != ridgecolorB);
}

double cartesian(double bearing)
{
	int conv = (90 - bearing);
	double converted = conv % 360;
	if (converted > 180)
		converted -= 360;
	else if (converted < -180)
		converted += 360;
	return converted;
}

unsigned char* importFile(char const* filename)
{
	FILE* fileptr;

	long filelen;

	fopen_s(&fileptr, filename, "rb");  // Open the file in binary mode
	fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
	filelen = ftell(fileptr);             // Get the current byte offset in the file
	rewind(fileptr);                      // Jump back to the beginning of the file

	buffer = (unsigned char*)malloc(filelen); // Enough memory for the file
	fread(buffer, filelen, 1, fileptr); // Read in the entire file
	fclose(fileptr); // Close the file

	return buffer;
}

unsigned char* getHgtFile(double north, double east)
{
	string filebase;
	int latitude = (int)north;
	int longitude = (int)east;
	//Console.WriteLine("latitude: " + latitude);


	if (latitude < 0)
	{
		filebase = "S";
		if (latitude < 10)
			filebase.append("0");
		filebase.append(std::to_string(abs(latitude) + 1));
		lat = latitude;
		d_lat = SAMPLE_SECONDS;
	}
	else
	{
		filebase = "N";
		if (latitude < 10)
			filebase.append("0");
		filebase.append(std::to_string(latitude));

		lat = latitude + 1;
		d_lat = -SAMPLE_SECONDS;
	}
	if (longitude < 0)
	{
		filebase += "W";
		if (longitude < 10)
		{
			filebase.append("00");
			
		}
		else if (longitude < 100)
			filebase.append("0");
		filebase.append(std::to_string(abs(longitude - 1)));
		filebase.append(".hgt");
		lon = longitude - 1;
	}
	else
	{
		filebase += "E";
		if (longitude < 10)
			filebase.append("00");
		else if (longitude < 100)
			filebase.append("0");
		filebase.append(std::to_string(longitude));
		filebase.append(".hgt");
		lon = longitude;
	}
	d_lon = SAMPLE_SECONDS;
	unsigned char* c;
	auto it = dict.find(filebase);
	if (it == dict.end())
	{
		string currentfile = DEM_DATA + filebase;
		c = importFile(currentfile.c_str());
		dict.insert(std::pair <std::string, unsigned char*>(filebase, c));
	}
	else
	{
		c = it->second;
	}
	return c;
}

double getDMinute(double Ddegree)
{
	return (Ddegree - (int)(Ddegree)) * 60;
}

double getDSecond(double Dmin)
{
	return ((Dmin - (int)Dmin) * 60);
}

int north_offset(int northM, int northS, double direction)
{
	int row = (int)(abs((northM * 60) + northS) / abs(direction));
	if (copysign(1, direction) != copysign(1, northM))
		row = SAMPLES_PER_ROW - row - 1;
	return row * BYTES_PER_ROW;
}

int east_offset(int eastM, int eastS, double direction)
{
	int offest = (int)abs((eastM * 60 + eastS) / abs(direction));
	if (copysign(1, direction) != copysign(1, eastM))
		offest = SAMPLES_PER_ROW - offest - 1;
	return offest * 2;
}

int getHeight(double north, double east)
{
	unsigned char* b = getHgtFile(north, east);
	double Nmin = getDMinute(north);
	double Emin = getDMinute(east);

	int offset = north_offset((int)Nmin, (int)getDSecond(Nmin), d_lat) + east_offset((int)Emin, (int)getDSecond(Emin), d_lon);
	unsigned char buff1;
	unsigned char buff2;
	buff1 = b[offset + 1];
	buff2 = b[offset];
	int ret = static_cast<int>(buff1 | buff2 << 8);
	return ret;
}

double ConvertDegreesToRadians(double degrees)
{
	double radians = (M_PI / 180) * degrees;
	return (radians);
}

double ConvertRadiansToDegrees(double radians) { double degrees = (180 / M_PI) * radians; return (degrees); }

double* moveOnEarth(double north, double east, double angle, double d_travel, bool bearing)
{
	double radian = (double)ConvertDegreesToRadians(angle);
	double* ret = new double[2];
	ret[0] = north + (double)sin(radian) * (d_travel / DEGREE_IN_METERS);
	ret[1] = east + (double)cos(radian) * (d_travel / DEGREE_IN_METERS);
	return ret;

}

int* look(double angle, double north, double east, int distance, double d_travel, double d_bearing)
{

	auto elevations = new int[((int)(distance / d_travel) + 1) * 3];

	double traversed = 0;
	double radian = (double)ConvertDegreesToRadians(angle);
	int count = 0;
	while (traversed < distance)
	{
		int elevation = getHeight(north, east);
		double CorrElevation = elevation - height;
		double theta = atan(CorrElevation / (step * count));

		int projected = (int)(round(theta / abs(d_bearing)));
		int proj2 = horizon - 1 - projected;

		elevations[count * 3] = elevation;
		elevations[count * 3 + 1] = projected;
		elevations[count * 3 + 2] = proj2;

		double* noea = moveOnEarth(north, east, angle, d_travel, false);
		north = noea[0];
		east = noea[1];
		traversed += d_travel;
		count++;
	}
	return elevations;
}

unsigned char* HSVtoRGB(float hue, float saturation, float value, float alpha)
{
	int hi1 = static_cast<int>(floor(hue / 60));
	int hi = hi1 % 6;
	double f = hue / 60 - floor(hue / 60);

	value = value * 255;
	int v = static_cast<int>(value);
	int p = static_cast<int>(value * (1 - saturation));
	int q = static_cast<int>(value * (1 - f * saturation));
	int t = static_cast<int>(value * (1 - (1 - f) * saturation));
	unsigned char* colorRet = new unsigned char[3];

	if (hi == 0)
	{
		colorRet[0] = v;
		colorRet[1] = t;
		colorRet[2] = p;

	}
	else if (hi == 1)
	{
		colorRet[0] = q;
		colorRet[1] = v;
		colorRet[2] = p;
	}
	else if (hi == 2)
	{
		colorRet[0] = p;
		colorRet[1] = v;
		colorRet[2] = t;
	}
	else if (hi == 3)
	{
		colorRet[0] = p;
		colorRet[1] = q;
		colorRet[2] = v;
	}
	else if (hi == 4)
	{
		colorRet[0] = t;
		colorRet[1] = p;
		colorRet[2] = v;
	}
	else
	{
		colorRet[0] = v;
		colorRet[1] = p;
		colorRet[2] = q;
	}
	return colorRet;
}

int main()
{
	double north = latitude;
	double east = longitude;

	int totalStep = (int)(viewrange / step) + 1;

	height += getHeight(north, east);
	bearing = (double)ConvertDegreesToRadians(cartesian(bearing));
	double halfspan = (double)ConvertDegreesToRadians(SPAN) / 2;
	double d_bearing = (double)asin((step) / viewrange);
	double angle = bearing + halfspan;
	int widhtSteps = (int)((abs(bearing - halfspan) - abs(angle)) / abs(d_bearing)) + 1;

	int** allLook{ new int* [widhtSteps] {} };

	size_t size = widhtSteps;

	parallel_for(size_t(0), size, [&](size_t i)
		{
			double selectAngle = angle - i * abs(d_bearing);
			allLook[i] = look((double)ConvertRadiansToDegrees(selectAngle), latitude, longitude, viewrange, step, d_bearing);
		});


	dict.clear();

	bitmap_image image(widhtSteps, image_height);

	parallel_for(size_t(0), size, [&](size_t i)
		{
			for (int y = 0; y < image_height; y++)
				image.set_pixel(i, y, 255, 255, 255);
		});


	parallel_for(size_t(0), size, [&](size_t i)
		{
			int x = i;
			int* t = allLook[i];
			int poinListCount = totalStep + 2;
			int** pointList = new int* [poinListCount];
			pointList[0] = &t[0];
			for (int j = 0; j < totalStep; j++)
			{
				pointList[j + 1] = &t[j * 3];
			}
			int* last = new int[3]{ 0,0,image_height };
			pointList[totalStep + 1] = last;
			for (int depth = totalStep + 2 - 3; depth > 0; depth--)
			{
				unsigned char ridgecolor[3] = { 0, 0, 0 };
				unsigned char* color = new unsigned char[3]{ 0, 0, 0 };
				int** context = new int* [4];
				for (int k = 0; k < 4; k++)
				{
					context[k] = pointList[depth - 1 + k];
				}
				//cout << context[1][2] << endl;
				if (context[1][2] == context[0][2])
					pointList[depth] = pointList[depth + 1];
				else if (context[1][1] > context[0][1])
				{
					bool testColor = false;
					if (testColor)
					{
						float divider = poinListCount / (float)(WHITE - DARKEST);
						unsigned char gray = (unsigned char)(depth / divider) + DARKEST;
						color[0] = color[1] = color[2] = gray;
					}
					else
					{
						float code = ((float)depth / (float)poinListCount) * 360.0f;
						color = HSVtoRGB(code, 1.0f, 1.0f, 1.0f);//Color.FromArgb(gray, gray, gray, gray);

					}
					int y = (int)max(0, context[1][2]);
					if (y < image_height)
					{
						if (y < context[2][2])
						{
							image.set_pixel(x, y, ridgecolor[0], ridgecolor[1], ridgecolor[2]);
						}
						else if ((y > context[2][2]))// || testPix(x, y, ridgecolor[0], ridgecolor[1], ridgecolor[2], image))
						{
							image.set_pixel(x, y, color[0], color[1], color[2]);
						}
						int start = y + 1;
						int end = start + context[0][2] + 1;

						for (int plot = start; plot < end; plot++)
						{
							if (plot < image_height)
							{
								image.set_pixel(x, plot, color[0], color[1], color[2]);
							}
						}
					}
				}

			}
		});

	image.save_image(OUTDIR+"testc.bmp");
	return 0;

}
