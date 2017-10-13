#pragma once

#include <string>
#include <vector>
#include <functional>

namespace tsk {

struct TaskResult {
  bool success;

  int errorCode;
  std::string errorMessage;
};

const TaskResult kTaskSuccess = {
  true
};

struct TaskSequenceResult {
  bool success;

  int failingTaskIndex;
  int errorCode;
  std::string errorMessage;
};

const TaskSequenceResult kTaskSequenceSuccess = {
  true
};

template<typename T>
struct Task {
  std::string description;
  std::function<TaskResult(T&)> func;
};

template<typename T>
TaskSequenceResult ExecuteTaskSequence(
  T &data,
  std::string sequenceDescription,
  std::vector<Task<T>> tasks
) {
  std::cout << "Running task sequence: " << sequenceDescription << std::endl;

  for (int i = 0; i < tasks.size(); ++i) {
    auto task = tasks[i];
    std::cout << "\t" << i << ". " << task.description << std::endl;
    TaskResult result = task.func(data);
    if (!result.success) {
      std::cerr
        << "Task sequence \""
        << sequenceDescription
        << "\" failed on index " << i
        << " with error code: " << result.errorCode
        << " and error message \"" << result.errorMessage << "\"" << std::endl;
      return {
        false,
        i,
        result.errorCode,
        std::move(result.errorMessage) // we don't really use result anymore
      };
    }
  }

  std::cout << "Finished task sequence: " << sequenceDescription << std::endl;
  return kTaskSequenceSuccess;
}

}
