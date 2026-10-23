// Included by the pinned SDL Apple driver and the macOS concurrency test.
// No joystick/device pointer crosses the asynchronous boundary.
static dispatch_queue_t VFCombatIOSHapticsQueue(void)
{
    static dispatch_queue_t queue;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        queue = dispatch_queue_create("vf-combat.ios.haptics", dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, 0));
    });
    return queue;
}

@interface VFCombatIOSRumbleContext : SDL3_RumbleContext
- (instancetype)initWithController:(GCController *)controller;
- (void)processLatest;
@end

@implementation VFCombatIOSRumbleContext {
    NSLock *_stateLock;
    GCController *_controller;
    SDL3_RumbleContext *_motors; // haptics queue only
    bool _closed, _scheduled, _hasTriggers;
    Uint16 _low, _high, _left, _right;
    uint64_t _revision;
    NSTimeInterval _retryAt; // haptics queue only
}

- (instancetype)initWithController:(GCController *)controller
{
    self = [super init];
    if (self) {
        _stateLock = [NSLock new];
        _controller = controller;
        _scheduled = true;
        // Prewarm engines without producing any nonzero haptic effect.
        dispatch_async(VFCombatIOSHapticsQueue(), ^{ [self processLatest]; });
    }
    return self;
}

- (bool)submitLow:(Uint16)low high:(Uint16)high left:(Uint16)left right:(Uint16)right triggers:(bool)triggers
{
    [_stateLock lock];
    if (_closed) {
        [_stateLock unlock];
        return false;
    }
    if (triggers) {
        _left = left; _right = right; _hasTriggers = true;
    } else {
        _low = low; _high = high;
    }
    ++_revision;
    if (!_scheduled) {
        _scheduled = true;
        dispatch_async(VFCombatIOSHapticsQueue(), ^{ [self processLatest]; });
    }
    [_stateLock unlock];
    return true;
}

- (bool)rumbleWithLowFrequency:(Uint16)low andHighFrequency:(Uint16)high
{
    return [self submitLow:low high:high left:0 right:0 triggers:false];
}

- (bool)rumbleLeftTrigger:(Uint16)left andRightTrigger:(Uint16)right
{
    return [self submitLow:0 high:0 left:left right:right triggers:true];
}

- (void)cleanup
{
    [_stateLock lock];
    _closed = true;
    _low = _high = _left = _right = 0;
    ++_revision;
    if (!_scheduled) {
        _scheduled = true;
        dispatch_async(VFCombatIOSHapticsQueue(), ^{ [self processLatest]; });
    }
    [_stateLock unlock];
}

- (void)processLatest
{
    @autoreleasepool {
        [_stateLock lock];
        bool closed = _closed;
        [_stateLock unlock];
        if (!closed && !_motors && NSProcessInfo.processInfo.systemUptime >= _retryAt) {
            NSTimeInterval started = NSProcessInfo.processInfo.systemUptime;
            _motors = IOS_JoystickCreateRumbleMotors(_controller);
            if (!_motors) _retryAt = NSProcessInfo.processInfo.systemUptime + 3.0;
            SDL_Log("vf-combat: iOS haptics background initialization %.1f ms, ready=%d", (NSProcessInfo.processInfo.systemUptime - started) * 1000.0, _motors != nil);
        }

        // Read again AFTER cold initialization. A stop/disconnect supersedes
        // every obsolete effect submitted while the engine was starting.
        [_stateLock lock];
        closed = _closed;
        uint64_t revision = _revision;
        Uint16 low = _low, high = _high, left = _left, right = _right;
        bool triggers = _hasTriggers;
        [_stateLock unlock];
        if (closed) {
            [_motors cleanup];
            _motors = nil;
            _controller = nil;
        } else if (_motors) {
            bool ok = [_motors rumbleWithLowFrequency:low andHighFrequency:high];
            if (triggers) ok = [_motors rumbleLeftTrigger:left andRightTrigger:right] && ok;
            if (!ok) {
                [_motors cleanup];
                _motors = nil;
                _retryAt = NSProcessInfo.processInfo.systemUptime + 1.0;
            }
        }
        [_stateLock lock];
        if ((!closed && _closed) || _revision != revision) {
            // Yield between snapshots so resets and other devices cannot starve.
            dispatch_async(VFCombatIOSHapticsQueue(), ^{ [self processLatest]; });
        } else {
            _scheduled = false;
        }
        [_stateLock unlock];
    }
}
@end
