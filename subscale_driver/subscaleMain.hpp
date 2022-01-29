#pragma once

#include "../src/Queue.hpp"
#include <functional>
#include <atomic>

enum QueuedFunctionType {
  Misc,
  Python
};

struct QueuedFunction {
  std::function<void(void)> f;
  const char *description;
  QueuedFunctionType type;
};

extern Queue<QueuedFunction, 8> mainDispatchQueue;
extern Queue<QueuedFunction, 16> radioDispatchQueue;
extern std::atomic<bool> isRunningPython;
extern std::atomic<bool> mainDispatchQueueRunning;
extern std::atomic<bool> mainDispatchQueueDrainThenStop;

enum Status : int {
  EnqueuingSIFT = 1,
  StartingSIFT,
  FinishedSIFT,
  IMUNotRespondingInCheckTakeoffCallback,
  TakeoffTimeRecordedInCheckTakeoffCallback,
  NotEnoughAccelInCheckTakeoffCallback,
  IMUNotRespondingInMainDeploymentCallback,
  SwitchingToSecondCamera,
  NotEnoughAccelInMainDeploymentCallback,
  AltitudeLessThanDesiredAmount,
  StoppingSIFTOnBackupTimeElapsed,
  TooMuchTimeElapsedWithoutMainDeployment_ThereforeForcingTrigger,
  RunningPython,
  IMUNotRespondingInPassIMUDataToSIFTCallback,
};
// Sets the status which sends on the radio
extern void reportStatus(Status status);
