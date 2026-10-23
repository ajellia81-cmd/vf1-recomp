#import <CoreHaptics/CoreHaptics.h>
#import <UIKit/UIKit.h>
#include "../../src/host/ios_haptics.h"
#include <algorithm>
#include <cstdio>

static dispatch_queue_t feedbackQueue() {
 static dispatch_queue_t q; static dispatch_once_t once;
 dispatch_once(&once, ^{ q=dispatch_queue_create("vf.combat.phone-haptics", dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL,QOS_CLASS_USER_INITIATED,0)); });
 return q;
}
@interface VFFightPhoneHaptics : NSObject
- (void)submitLow:(float)low high:(float)high milliseconds:(unsigned)ms active:(BOOL)active close:(BOOL)close;
- (void)processLatest;
- (void)stop;
- (BOOL)prepare;
- (void)invalidate;
- (void)dispose;
@end
@implementation VFFightPhoneHaptics {
 NSLock *_lock;
 BOOL _active, _closed, _scheduled;
 uint64_t _revision;
 float _low, _high; unsigned _ms; NSTimeInterval _submitted;
 // Engine and player are owned exclusively by feedbackQueue().
 CHHapticEngine *_engine;
 id<CHHapticPatternPlayer> _player;
 NSTimeInterval _retryAt;
}
- (instancetype)init { if ((self=[super init])) _lock=[NSLock new]; return self; }
- (void)submitLow:(float)low high:(float)high milliseconds:(unsigned)ms active:(BOOL)active close:(BOOL)close {
 [_lock lock];
 if (!_closed) {
  _active=active; _closed=close; _low=low; _high=high; _ms=ms;
  _submitted=NSProcessInfo.processInfo.systemUptime; ++_revision;
  if (!_scheduled) { _scheduled=YES; dispatch_async(feedbackQueue(), ^{ [self processLatest]; }); }
 }
 [_lock unlock];
}
- (void)stop {
 [_lock lock]; BOOL active=_active; [_lock unlock];
 [self submitLow:0 high:0 milliseconds:0 active:active close:NO];
}
- (BOOL)prepare {
 if (_player) return YES;
 if (NSProcessInfo.processInfo.systemUptime < _retryAt || !CHHapticEngine.capabilitiesForHardware.supportsHaptics) return NO;
 NSTimeInterval started=NSProcessInfo.processInfo.systemUptime;
 if (!_engine) {
  _engine=[[CHHapticEngine alloc] initAndReturnError:nil];
  _engine.playsHapticsOnly=YES; _engine.autoShutdownEnabled=NO;
  __weak VFFightPhoneHaptics *weakSelf=self;
  _engine.resetHandler=^{ dispatch_async(feedbackQueue(), ^{ [weakSelf invalidate]; }); };
  _engine.stoppedHandler=^(CHHapticEngineStoppedReason reason) { (void)reason; dispatch_async(feedbackQueue(), ^{ [weakSelf invalidate]; }); };
 }
 if (![_engine startAndReturnError:nil]) { _retryAt=NSProcessInfo.processInfo.systemUptime+2; return NO; }
 NSArray *parameters=@[
  [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity value:1],
  [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness value:0.5]
 ];
 CHHapticEvent *impact=[[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticTransient parameters:parameters relativeTime:0];
 CHHapticEvent *tail=[[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticContinuous parameters:parameters relativeTime:0 duration:0.25];
 CHHapticPattern *pattern=[[CHHapticPattern alloc] initWithEvents:@[impact,tail] parameters:@[] error:nil];
 _player=[_engine createPlayerWithPattern:pattern error:nil];
 if (!_player) _retryAt=NSProcessInfo.processInfo.systemUptime+2;
 fprintf(stderr,"haptics: phone background preparation %.1f ms, ready=%d\n",(NSProcessInfo.processInfo.systemUptime-started)*1000,_player!=nil);
 return _player!=nil;
}
- (void)invalidate {
 _player=nil;
 // Recovery prepares resources silently; a previous impact is never replayed.
 [_lock lock]; BOOL active=_active&&!_closed; [_lock unlock];
 if (active) [self prepare];
}
- (void)dispose {
 [_player stopAtTime:CHHapticTimeImmediate error:nil]; _player=nil;
 _engine.resetHandler=nil; _engine.stoppedHandler=nil;
 [_engine stopWithCompletionHandler:nil]; _engine=nil;
}
- (void)processLatest {
 @autoreleasepool {
  [_lock lock]; BOOL active=_active&&!_closed; [_lock unlock];
  if (active) [self prepare];
  // Re-read AFTER cold initialization so stop/background supersedes pending hits.
  [_lock lock]; uint64_t revision=_revision;
  active=_active&&!_closed; float low=_low,high=_high; unsigned ms=_ms;
  NSTimeInterval age=NSProcessInfo.processInfo.systemUptime-_submitted;
  [_lock unlock];
  if (!active) [self dispose];
  else {
   [_player stopAtTime:CHHapticTimeImmediate error:nil];
   if (_player && ms && age<=0.1 && std::max(low,high)>0) {
    float intensity=std::clamp(std::max(low,high),0.0f,1.0f);
    float sharpness=std::clamp(high/std::max(0.01f,low+high),0.0f,1.0f)-0.5f;
    double duration=std::clamp(ms,10u,250u)/1000.0;
    CHHapticParameterCurve *fade=[[CHHapticParameterCurve alloc] initWithParameterID:CHHapticDynamicParameterIDHapticIntensityControl controlPoints:@[
     [[CHHapticParameterCurveControlPoint alloc] initWithRelativeTime:0 value:intensity],
     [[CHHapticParameterCurveControlPoint alloc] initWithRelativeTime:duration value:0]
    ] relativeTime:0];
    [_player sendParameters:@[[[CHHapticDynamicParameter alloc] initWithParameterID:CHHapticDynamicParameterIDHapticSharpnessControl value:sharpness relativeTime:0]] atTime:CHHapticTimeImmediate error:nil];
    if (![_player scheduleParameterCurve:fade atTime:CHHapticTimeImmediate error:nil] || ![_player startAtTime:CHHapticTimeImmediate error:nil]) { _player=nil; _retryAt=NSProcessInfo.processInfo.systemUptime+1; }
   }
  }
  [_lock lock];
  if (_revision!=revision) dispatch_async(feedbackQueue(), ^{ [self processLatest]; });
  else _scheduled=NO;
  [_lock unlock];
 }
}
@end
static VFFightPhoneHaptics *phone; // main thread only; native state stays on feedbackQueue().
void ios_haptics_prepare() {
 NSCAssert(NSThread.isMainThread,@"Main-thread submission required");
 if (UIApplication.sharedApplication.applicationState!=UIApplicationStateActive) return;
 if (!phone) phone=[VFFightPhoneHaptics new];
 [phone submitLow:0 high:0 milliseconds:0 active:YES close:NO];
}
void ios_haptics_play(float low,float high,unsigned ms) {
 NSCAssert(NSThread.isMainThread,@"Main-thread submission required");
 if (UIApplication.sharedApplication.applicationState!=UIApplicationStateActive) return;
 if (!phone) phone=[VFFightPhoneHaptics new];
 [phone submitLow:low high:high milliseconds:ms active:YES close:NO];
}
void ios_haptics_stop() { [phone stop]; }
void ios_haptics_suspend() { [phone submitLow:0 high:0 milliseconds:0 active:NO close:NO]; }
void ios_haptics_shutdown() { [phone submitLow:0 high:0 milliseconds:0 active:NO close:YES]; phone=nil; }
