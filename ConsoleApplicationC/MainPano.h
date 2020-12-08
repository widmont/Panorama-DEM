#include <string>
#include <math.h>
#include <map>
#include <ppl.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>
using namespace std;

const int SAMPLE_SECONDS = 1;
string DEM_DATA = "C://tests//N46E007//";
string OUTDIR = "C://tests//";
const int DEGREE_IN_SECONDS = 60 * 60;

constexpr double SPAN = 300.0f;
const int WHITE = 255;
const int DARKEST = 10;
const int BLACK = 0;
constexpr double GLOBE = 3959.0;

constexpr double RADIUS = 6371000.0;

constexpr double M_PI = 3.14159265358979323846;

const int SAMPLES_PER_ROW = (DEGREE_IN_SECONDS / SAMPLE_SECONDS) + 1;
const int DEGREE_IN_SAMPLES = DEGREE_IN_SECONDS / SAMPLE_SECONDS;
const int BYTES_PER_ROW = SAMPLES_PER_ROW * 2;
const bool OCEANFRONT = false;
constexpr double DEGREE_IN_METERS = (double)((RADIUS * 2 * M_PI) / 360.0);
constexpr double SAMPLE_IN_METERS = DEGREE_IN_METERS / (60 * (60 / SAMPLE_SECONDS));

constexpr double step = SAMPLE_IN_METERS;

constexpr double longitude = 7.51019f;
constexpr double latitude = 46.3363f;
double height = 200.0f;
double bearing = 180;
int viewrange = 70000;

const int image_height = 720;
const int horizon = (int)(image_height / 2);

unsigned char* buffer;

std::map<std::string, unsigned char*> dict;

int lat;
int lon;
int d_lat;
int d_lon;
