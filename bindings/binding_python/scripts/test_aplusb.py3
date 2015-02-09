import winc
import os
import msvcrt
import random
import time

c = winc.Container()

def call_aplusb(a, b):
  stdin_r, stdin_w = os.pipe()
  stdout_r, stdout_w = os.pipe()
  t = c.spawn(os.getcwd() + u"\\payload_aplusb.exe",
              stdin_handle = msvcrt.get_osfhandle(stdin_r),
              stdout_handle = msvcrt.get_osfhandle(stdout_w))
  os.close(stdin_r)
  os.close(stdout_w)
  t.start()
  os.write(stdin_w, bytes(str(a) + " " + str(b) + "\n", "ascii"))
  os.close(stdin_w)
  ret = os.read(stdout_r, 16)
  os.close(stdout_r)
  return int(ret)

last = time.clock()
cnt = 0
while True:
  a = random.randint(0, 32767)
  b = random.randint(0, 32767)
  x = call_aplusb(a, b)
  if x != a + b:
    raise Exception("Math error")
  cnt += 1
  cur = time.clock()
  if cur - last >= 1:
    print("Spawned", cnt, "processes in 1 second")
    last, cnt = cur, 0
