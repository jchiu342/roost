//
// Created by Jeremy on 1/17/2022.
//

#ifndef ROOST_GTP_H
#define ROOST_GTP_H
#include <memory>
#include "player/AbstractPlayer.h"

class GTP {
public:
  explicit GTP(std::shared_ptr<AbstractPlayer> engine);
  void run();
private:
  std::shared_ptr<AbstractPlayer> engine_;
};


#endif //ROOST_GTP_H
