#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint16_t Uint16;
#define SDL_Log(...) ((void)0)
static void check(bool value, const char *message) {
    if (!value) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
@interface GCController : NSObject @end
@implementation GCController @end
static NSMutableArray<NSString *> *events;
static atomic_int initialized, disposed;
static atomic_bool gateInit, gateSend, failInit;
static dispatch_semaphore_t initEntered, initRelease, sendEntered, sendRelease;

@interface SDL3_RumbleContext : NSObject
- (bool)rumbleWithLowFrequency:(Uint16)low andHighFrequency:(Uint16)high;
- (bool)rumbleLeftTrigger:(Uint16)left andRightTrigger:(Uint16)right;
- (void)cleanup;
@end
@implementation SDL3_RumbleContext
- (bool)rumbleWithLowFrequency:(Uint16)low andHighFrequency:(Uint16)high {
    check(!NSThread.isMainThread, "native player work must be off main thread");
    if (atomic_exchange(&gateSend, false)) {
        dispatch_semaphore_signal(sendEntered);
        check(dispatch_semaphore_wait(sendRelease, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)) == 0, "send gate timeout");
    }
    [events addObject:[NSString stringWithFormat:@"rumble:%u:%u", low, high]];
    return true;
}
- (bool)rumbleLeftTrigger:(Uint16)left andRightTrigger:(Uint16)right {
    check(!NSThread.isMainThread, "trigger player work must be off main thread");
    [events addObject:[NSString stringWithFormat:@"trigger:%u:%u", left, right]];
    return true;
}
- (void)cleanup {
    check(!NSThread.isMainThread, "native cleanup must be off main thread");
    atomic_fetch_add(&disposed, 1);
    [events addObject:@"cleanup"];
}
@end

static SDL3_RumbleContext *IOS_JoystickCreateRumbleMotors(GCController *controller) {
    check(!NSThread.isMainThread, "cold engine creation must be off main thread");
    atomic_fetch_add(&initialized, 1);
    if (atomic_exchange(&gateInit, false)) {
        dispatch_semaphore_signal(initEntered);
        check(dispatch_semaphore_wait(initRelease, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)) == 0, "initialization gate timeout");
    }
    return atomic_exchange(&failInit, false) ? nil : [SDL3_RumbleContext new];
}

#include "../../ios/Scripts/ios_rumble_async.inc"

static void flush(void) {
    for (int i = 0; i < 3; ++i) dispatch_sync(VFCombatIOSHapticsQueue(), ^{});
}
static void waitFor(dispatch_semaphore_t s) {
    check(dispatch_semaphore_wait(s, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)) == 0, "worker did not reach gate");
}
static void reset(void) {
    flush(); events = [NSMutableArray new];
    atomic_store(&initialized, 0); atomic_store(&disposed, 0);
    atomic_store(&gateInit, false); atomic_store(&gateSend, false); atomic_store(&failInit, false);
    initEntered = dispatch_semaphore_create(0); initRelease = dispatch_semaphore_create(0);
    sendEntered = dispatch_semaphore_create(0); sendRelease = dispatch_semaphore_create(0);
}
int main(void) {
    @autoreleasepool {
        reset();
        atomic_store(&gateInit, true);
        VFCombatIOSRumbleContext *r = [[VFCombatIOSRumbleContext alloc] initWithController:[GCController new]];
        waitFor(initEntered);
        for (int i = 1; i <= 1000; ++i) check([r rumbleWithLowFrequency:i andHighFrequency:i], "submission accepted while engine is blocked");
        [r rumbleWithLowFrequency:0 andHighFrequency:0];
        check(events.count == 0, "cold engine has not emitted a stale effect");
        dispatch_semaphore_signal(initRelease); flush();
        check(events.count == 1 && [events.lastObject isEqual:@"rumble:0:0"], "latest stop supersedes cold-start effects; backlog is bounded");
        [r rumbleWithLowFrequency:111 andHighFrequency:222];
        [r rumbleLeftTrigger:333 andRightTrigger:444]; flush();
        check([events containsObject:@"rumble:111:222"] && [events containsObject:@"trigger:333:444"], "motor amplitudes and channels preserved");
        [r cleanup]; flush(); check(atomic_load(&disposed) == 1, "native resources disposed once");
        check(![r rumbleWithLowFrequency:1 andHighFrequency:1], "closed device rejects effects");

        reset(); atomic_store(&gateInit, true);
        r = [[VFCombatIOSRumbleContext alloc] initWithController:[GCController new]]; waitFor(initEntered);
        [r rumbleWithLowFrequency:123 andHighFrequency:456]; [r cleanup];
        dispatch_semaphore_signal(initRelease); flush();
        check(events.count == 1 && [events.lastObject isEqual:@"cleanup"], "disconnect during initialization never plays queued vibration");

        reset(); r = [[VFCombatIOSRumbleContext alloc] initWithController:[GCController new]]; flush();
        atomic_store(&gateSend, true); [r rumbleWithLowFrequency:7 andHighFrequency:8]; waitFor(sendEntered);
        [r cleanup]; // Must return while native player is still blocked.
        dispatch_semaphore_signal(sendRelease); flush();
        check([events.lastObject isEqual:@"cleanup"] && atomic_load(&disposed) == 1, "disconnect during player update safely disposes on same serial queue");

        reset(); atomic_store(&failInit, true);
        r = [[VFCombatIOSRumbleContext alloc] initWithController:[GCController new]]; flush();
        for (int i = 0; i < 1000; ++i) [r rumbleWithLowFrequency:9 andHighFrequency:10];
        flush(); check(atomic_load(&initialized) == 1, "failed initialization does not busy-retry every game frame");
        [NSThread sleepForTimeInterval:3.1]; [r rumbleWithLowFrequency:11 andHighFrequency:12]; flush();
        check(atomic_load(&initialized) == 2 && [events containsObject:@"rumble:11:12"], "failed engine can recover off-thread after backoff");
        [r cleanup]; flush();
        puts("iOS rumble queue passed: nonblocking cold start, coalescing, stop, disconnect, channels, bounded retry/recovery");
    }
    return 0;
}
