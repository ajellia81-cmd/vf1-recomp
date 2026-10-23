#import <CoreHaptics/CoreHaptics.h>
#import <UIKit/UIKit.h>
#include "../../src/host/ios_haptics.h"
#include <algorithm>

@interface VFFightPhoneHaptics : NSObject
@property(nonatomic, strong) CHHapticEngine *engine;
@property(nonatomic, strong) id<CHHapticPatternPlayer> player;
@end
@implementation VFFightPhoneHaptics
- (void)stop {
	[self.player stopAtTime:CHHapticTimeImmediate error:nil]; self.player = nil;
}
- (void)playLow:(float)low high:(float)high milliseconds:(unsigned)ms {
	if (UIApplication.sharedApplication.applicationState != UIApplicationStateActive) return;
	if (!CHHapticEngine.capabilitiesForHardware.supportsHaptics) return;
	if (!self.engine) {
		self.engine = [[CHHapticEngine alloc] initAndReturnError:nil];
		self.engine.playsHapticsOnly = YES;
		self.engine.autoShutdownEnabled = YES;
		__weak VFFightPhoneHaptics *weakSelf = self;
		self.engine.resetHandler = ^{ dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf stop]; }); };
		self.engine.stoppedHandler = ^(CHHapticEngineStoppedReason reason) {
			(void)reason; dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf stop]; });
		};
	}
	if (![self.engine startAndReturnError:nil]) return;
	[self stop]; // Latest impact replaces the old tail; never accumulate delayed hits.
	const float intensity = std::clamp(std::max(low, high), 0.0f, 1.0f);
	const float sharpness = std::clamp(high / std::max(0.01f, low + high), 0.0f, 1.0f);
	NSArray *parameters = @[
		[[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity value:intensity],
		[[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness value:sharpness]
	];
	const double duration = std::clamp(ms, 10u, 250u) / 1000.0;
	CHHapticEvent *impact = [[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticTransient parameters:parameters relativeTime:0];
	CHHapticEvent *tail = [[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticContinuous parameters:parameters relativeTime:0.005 duration:duration];
	CHHapticParameterCurve *fade = [[CHHapticParameterCurve alloc] initWithParameterID:CHHapticDynamicParameterIDHapticIntensityControl controlPoints:@[
		[[CHHapticParameterCurveControlPoint alloc] initWithRelativeTime:0 value:1],
		[[CHHapticParameterCurveControlPoint alloc] initWithRelativeTime:duration value:0]
	] relativeTime:0];
	CHHapticPattern *pattern = [[CHHapticPattern alloc] initWithEvents:@[impact, tail] parameterCurves:@[fade] error:nil];
	if (!pattern) return;
	self.player = [self.engine createPlayerWithPattern:pattern error:nil];
	[self.player startAtTime:CHHapticTimeImmediate error:nil];
}
@end

static VFFightPhoneHaptics *phone;
void ios_haptics_play(float low, float high, unsigned ms) {
	NSCAssert(NSThread.isMainThread, @"Haptics must be submitted on the main thread");
	if (!phone) phone = [VFFightPhoneHaptics new];
	[phone playLow:low high:high milliseconds:ms];
}
void ios_haptics_stop() { [phone stop]; }
void ios_haptics_shutdown() {
	[phone stop]; phone.engine.resetHandler = nil; phone.engine.stoppedHandler = nil;
	[phone.engine stopWithCompletionHandler:nil]; phone = nil;
}
