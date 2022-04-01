#include <cmath>
#include <cstdio>

double computeAltitude(double kilopascals) {
  double altitudeFeet = 145366.45 * (1.0 - std::pow(10.0 * kilopascals / 1013.25, 0.190284)); // https://en.wikipedia.org/wiki/Pressure_altitude
  return altitudeFeet;
}

int main() {
  double alt;
  scanf("%lf", &alt);
  printf("%lf\n", computeAltitude(alt));
  return 0;
}
