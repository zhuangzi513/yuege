#ifndef FORECASTER_H
#define FORECASTER_H

#include <list>
#include <string>


class Forecaster {
  public:
    Forecaster();
    ~Forecaster();

    // Filter the details through TurnOver and compute the HintRate
    bool forecasteThroughTurnOver(const std::string& aDBName);

  private:
};


#endif
