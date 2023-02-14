#include <cmath>
#include <time.h>

/*
Seattle sun is essentially a screen that mimics the sun in seattle in real time.
The applciation can be modified to work regardles of location. It just happens to be the case
that i live in Seattle.
*/

// https://gml.noaa.gov/grad/solcalc/solareqns.PDF
class SunBrightnessConvertor {
  public:

  SunBrightnessConvertor(double currentLatitude, double currentLongitude):
  latitude(currentLatitude), longitude(currentLongitude) {}

  int SunBrightnessNow() const {
    // get time now
    ime_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);
    int dayOfYear = aTime->tm_yday;
    int currentYear = aTime->tm_year + 1900; // Year is # years since 1900
    int hour = aTime->tm_hour;
    double fractionalYear = calcFractionalYear(currentYear, dayOfYear, hour);
    double equationOfTime = calcEquationOfTime(fractionalYear);
    double solarDeclinationAngle = calcSolarDeclinationAngle(fractionalYear);
    double unsignedHourAngle = calcUnsignedHourAngle(solarDeclinationAngle);
    double solarNoon = calcSolarNoon(equationOfTime);
    double duskUtc = calcDuskUtc(unsignedHourAngle, equationOfTime);
    double dawnUtc = calcDawnUtc(unsignedHourAngle, equationOfTime);
    // If the time now is before duskUtc or time now is after dawnUTC return 0
    // draw a Hill like distrbution with evenly spaced increments and decrements where solar noon is peak
  }

  private:
  
  double latitude;
  double longitude;

  const double pi = 3.14159265358979323846;
  const double degrees_to_radians = pi / 180.0;
  const double radians_to_degrees = 180.0 / pi;
  const double sunriseZenith = 90.83; // degrees

  double calcFractionalYear(int currentYear, int dayOfYear, int hour) const {
    int daysInYear = currentYear % 4 == 0 ? 366 : 365;
    return ((2*pi)/daysInYear) * (dayOfYear - 1 + ((hour-12)/24));
  }

  double calcEquationOfTime(double fractionalYear) const {
    return 229.18 * (0.000075 + 0.001868*cos(fractionalYear) – 0.032077*sin(fractionalYear) – 0.014615*cos(2*fractionalYear) – 0.040849sin(2*fractionalYear) );
  }
  
  double calcSolarDeclinationAngle(double fractionalYear) const {
    return 0.006918 – 0.399912*cos(fractionalYear) + 0.070257*sin(fractionalYear) – 
    0.006758*cos(2*fractionalYear) + 0.000907*sin(2*fractionalYear) – 0.002697*cos(3*fractionalYear) +
    0.00148*sin(3*fractionalYear);
  }

  double calcUnsignedHourAngle(double solarDeclination) const {
    return arccos( (cos(sunriseZenith)/(cos(latitude)*cos(solarDeclination))) - (tan(latitude))*tan(solarDeclination) );
  }

  double calcSolarNoon(doubel equationOfTime) const {
    return 720 – 4*longitude – eqtime
  };

  // Dusk and Dawn calculations are based on civil twilight
  double calcDuskUtc(double hourAngle, double equationOfTime) const {
    return 720 – 4*(longitude + (-1*hourAngle)) – equationOfTime;
  }

  double calcDawnUtc(double hourAngle, double equationOfTime) const {
    return 720 – 4*(longitude + hourAngle) – equationOfTime;
  }
};

void setup() {
  // put your setup code here, to run once:
  
}

void loop() {
  
}
