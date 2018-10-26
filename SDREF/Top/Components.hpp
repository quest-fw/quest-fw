#ifndef __SDREF_COMPONENTS_HEADER__
#define __SDREF_COMPONENTS_HEADER__
void constructSDREFArchitecture(void);
void exitTasks(void);

#include <Svc/ActiveRateGroup/ActiveRateGroupImpl.hpp>
#include <Svc/RateGroupDriver/RateGroupDriverImpl.hpp>

#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Svc/CmdSequencer/CmdSequencerImpl.hpp>
#include <Svc/PassiveConsoleTextLogger/ConsoleTextLoggerImpl.hpp>
#include <Svc/ActiveLogger/ActiveLoggerImpl.hpp>
#include <Svc/LinuxTime/LinuxTimeImpl.hpp>
#include <Svc/TlmChan/TlmChanImpl.hpp>
#include <Svc/PrmDb/PrmDbImpl.hpp>
#include <Fw/Obj/SimpleObjRegistry.hpp>
#include <Svc/FileUplink/FileUplink.hpp>
#include <Svc/FileDownlink/FileDownlink.hpp>
#include <Svc/BufferManager/BufferManager.hpp>
#include <Svc/Health/HealthComponentImpl.hpp>

#include <Svc/SocketGndIf/SvcSocketGndIfImpl.hpp>

#include <SnapdragonFlight/HexRouter/HexRouterComponentImpl.hpp>
#include <HLProc/HLRosIface/HLRosIfaceComponentImpl.hpp>
#include <HLProc/LLRouter/LLRouterComponentImpl.hpp>

#include <SDREF/Top/TargetInit.hpp>
#include <Svc/AssertFatalAdapter/AssertFatalAdapterComponentImpl.hpp>
#include <Svc/FatalHandler/FatalHandlerComponentImpl.hpp>

#include <Gnc/Ctrl/ActuatorAdapter/ActuatorAdapterComponentImpl.hpp>

#include <Drv/LinuxSerialDriver/LinuxSerialDriverComponentImpl.hpp>

extern Drv::LinuxSerialDriverComponentImpl serialDriverLL;
extern Drv::LinuxSerialDriverComponentImpl serialDriverDebug;

extern Svc::RateGroupDriverImpl rgDrv;
extern Svc::ActiveRateGroupImpl rgTlm;
extern Svc::ActiveRateGroupImpl rgXfer;
extern Svc::CmdSequencerComponentImpl cmdSeq;
extern Svc::SocketGndIfImpl sockGndIf;
extern Svc::ConsoleTextLoggerImpl textLogger;
extern Svc::ActiveLoggerImpl eventLogger;
extern Svc::LinuxTimeImpl linuxTime;
extern Svc::TlmChanImpl chanTlm;
extern Svc::CommandDispatcherImpl cmdDisp;
extern Svc::PrmDbImpl prmDb;
extern Svc::AssertFatalAdapterComponentImpl fatalAdapter;
extern Svc::FatalHandlerComponentImpl fatalHandler;
extern SnapdragonFlight::HexRouterComponentImpl hexRouter;
extern HLProc::HLRosIfaceComponentImpl sdRosIface;
extern HLProc::LLRouterComponentImpl llRouter;
extern Gnc::ActuatorAdapterComponentImpl actuatorAdapter;

#endif
