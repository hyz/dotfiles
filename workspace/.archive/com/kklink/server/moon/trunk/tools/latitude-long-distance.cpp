#include <iostream>
#include <math.h>
#include <boost/lexical_cast.hpp>

inline double rad(double d)
{
    const double pi = 3.14159265;
    return d * pi / 180.0;
}

inline double GetDistance(double lat1, double lng1, double lat2, double lng2)
{
    const double EARTH_RADIUS = 6378.137;
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(lng1) - rad(lng2);
    double s = 2 * asin(sqrt(pow(sin(a/2),2)+cos(radLat1)*cos(radLat2)*pow(sin(b/2),2)));
    s = s * EARTH_RADIUS;
    s = round(s * 10000) / 10000;
    return s;
}

struct Position
{
    Position(double a, double b):lat1(a),lng1(b)
    {}

    double get_distance(double latitude, double longtitude) const
    {
        return GetDistance(lat1, lng1, latitude, longtitude);
    }
    double lat1,lng1;
};

int main(int argc, char *const argv[])
{
    if (argc != 3)
        return 1;

    double x0 = boost::lexical_cast<double>(getenv("latitude"));
    double y0 = boost::lexical_cast<double>(getenv("longtitude"));
    double x1 = boost::lexical_cast<double>(argv[1]);
    double y1 = boost::lexical_cast<double>(argv[2]);

    Position px(x0,y0);
    std::cout << px.get_distance(x1, y1) << "\n";

    return 0;
}

