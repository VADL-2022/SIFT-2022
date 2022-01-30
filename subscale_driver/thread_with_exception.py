# https://www.geeksforgeeks.org/python-different-ways-to-kill-a-thread/

# Python program raising
# exceptions in a python
# thread
 
import threading
import ctypes
import time
  
class thread_with_exception(threading.Thread):
    def __init__(self, name, **kwargs):
        threading.Thread.__init__(self, None, kwargs)
        self.name = name
             
    # def run(self):
 
    #     # target function of the thread class
    #     try:
    #         while True:
    #             print('running ' + self.name)
    #     finally:
    #         print('ended')
          
    def get_id(self):
 
        # returns id of the respective thread
        if hasattr(self, '_thread_id'):
            return self._thread_id
        for id, thread in threading._active.items():
            if thread is self:
                return id
  
    def raise_exception(self):
        thread_id = self.get_id()
        res = ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id,
              ctypes.py_object(SystemExit))
        if res > 1:
            ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id, 0)
            print('Exception raise failure')
