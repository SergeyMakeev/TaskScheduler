rd /Q /S "./VisualStudio"
rd /Q /S "./Scheduler/Include/Platform/Orbis"
del /S *.cxx
sunifdef.exe --replace --recurse --filter cpp,h,inl --discard drop --undef MT_PLATFORM_DURANGO --undef MT_PLATFORM_ORBIS --undef _XBOX_ONE --undef __ORBIS__ ./Scheduler
sunifdef.exe --replace --recurse --filter cpp,h,inl --discard drop --undef MT_PLATFORM_DURANGO --undef MT_PLATFORM_ORBIS --undef _XBOX_ONE --undef __ORBIS__ ./SchedulerTests