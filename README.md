# CompatKiller
This application monitors and kills the annoying CompatTelRunner.exe Windows process.

First, I don't want any (compatibility) telemety at all.
Second, it burns my notebook CPU and drains its battery.

I tried to disable this Windows functionality in a proper way, but without luck. So, I wrote this app.

After compilation, make a scheduled task or use https://nssm.cc/ manager to install is as a service:

nssm install CompatKiller CompatKiller.exe

net start CompatKiller

Cheers,
Nikolai
