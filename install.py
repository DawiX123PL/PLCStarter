#!/bin/python3
import os

# how to autostart script, tutorial from 
# https://learn.sparkfun.com/tutorials/how-to-run-a-raspberry-pi-program-on-startup/all

# nice hack from https://stackoverflow.com/questions/2817264/how-to-get-the-parent-dir-location
script_path = os.path.dirname(__file__)
exec_path = os.path.join(script_path, "build/PLCStarter")

print(exec_path)

unit_file_content = """[Unit]
Description=PLCStarter
Wants=network-online.target
After=network.target network-online.target

[Service]
ExecStart={}
Restart=always
RestartSec=10s
KillMode=process
TimeoutSec=infinity

[Install]
WantedBy=multi-user.target""".format(exec_path)


# write unit file to /lib/systemd/system/PLCStarter.service
print("writing unit file...")
f = open("/lib/systemd/system/PLCStarter.service", "w")
f.write(unit_file_content)
f.close()

# restart systemctl
print("restarting systemctl...")
os.system("systemctl daemon-reload")

# enable server service
print("enable server service...")
os.system("systemctl enable PLCStarter.service")
