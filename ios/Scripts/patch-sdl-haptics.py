"""Keep cold Core Haptics work outside the render thread AND SDL joystick lock."""
from pathlib import Path
import sys

p = Path(sys.argv[1]) / 'src/joystick/apple/SDL_mfijoystick.m'
s = p.read_text()

def edit(old, new, count=1):
    global s
    assert s.count(old) == count, (old[:100], s.count(old))
    s = s.replace(old, new)

edit('static bool IOS_JoystickOpen(SDL_Joystick *joystick, int device_index)', '''#if defined(SDL_PLATFORM_IOS) && defined(SDL_JOYSTICK_MFI)
@class SDL3_RumbleContext;
static SDL3_RumbleContext *IOS_JoystickInitRumble(GCController *controller);
static dispatch_queue_t VFCombatIOSHapticsQueue(void);
#endif

static bool IOS_JoystickOpen(SDL_Joystick *joystick, int device_index)''')
anchor = '''                    }
                }
            }
        }
#endif // SDL_JOYSTICK_MFI
    }
    if (device->is_siri_remote)'''
edit(anchor, '''                    }
                }
#ifdef SDL_PLATFORM_IOS
                if (!device->rumble && SDL_GetBooleanProperty(SDL_GetJoystickProperties(joystick), SDL_PROP_JOYSTICK_CAP_RUMBLE_BOOLEAN, false)) {
                    // Lightweight queue/context creation only; engines start off-thread.
                    device->rumble = (void *)CFBridgingRetain(IOS_JoystickInitRumble(controller));
                }
#endif
            }
        }
#endif // SDL_JOYSTICK_MFI
    }
    if (device->is_siri_remote)''')
edit('static SDL3_RumbleContext *IOS_JoystickInitRumble(GCController *controller)\n{', 'static SDL3_RumbleContext *IOS_JoystickCreateRumbleMotors(GCController *controller)\n{')
inc = (Path(__file__).parent / 'ios_rumble_async.inc').read_text()
edit('\n#endif // SDL_JOYSTICK_MFI\n\nstatic bool IOS_JoystickRumble(', '''
#ifdef SDL_PLATFORM_IOS
''' + inc + '''
static SDL3_RumbleContext *IOS_JoystickInitRumble(GCController *controller)
{
    return [[VFCombatIOSRumbleContext alloc] initWithController:controller];
}
#else
#define IOS_JoystickInitRumble IOS_JoystickCreateRumbleMotors
#endif

#endif // SDL_JOYSTICK_MFI

static bool IOS_JoystickRumble(''')
# Engine callbacks can arrive on arbitrary threads. Serialize them with player
# operations and disposal, without ever waiting on the rendering thread.
for opening, ending in [
    ('        self.engine.stoppedHandler = ^(CHHapticEngineStoppedReason stoppedReason) {', '          _this.engine = nil;\n        };'),
    ('        self.engine.resetHandler = ^{', '          [_this.engine startAndReturnError:nil];\n        };')]:
    edit(opening, opening + '\n#ifdef SDL_PLATFORM_IOS\n          dispatch_async(VFCombatIOSHapticsQueue(), ^{\n#endif')
    edit(ending, ending.replace('\n        };', '\n#ifdef SDL_PLATFORM_IOS\n          });\n#endif\n        };'))
edit('                [self.engine stopWithCompletionHandler:nil];', '''#ifdef SDL_PLATFORM_IOS
                self.engine.stoppedHandler = nil;
                self.engine.resetHandler = nil;
#endif
                [self.engine stopWithCompletionHandler:nil];''')
edit('        [self.engine startAndReturnError:&error];', '''#ifdef SDL_PLATFORM_IOS
        self.engine.playsHapticsOnly = YES;
#endif
        [self.engine startAndReturnError:&error];''')
edit('    [self.highFrequencyMotor cleanup];\n}', '''    [self.highFrequencyMotor cleanup];
#ifdef SDL_PLATFORM_IOS
    [self.leftTriggerMotor cleanup];
    [self.rightTriggerMotor cleanup];
#endif
}''')
p.write_text(s)
print('iOS haptics: background prewarm, coalesced serial updates, serialized reset/cleanup; no native engine work under SDL joystick lock')
