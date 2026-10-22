#import <GameController/GameController.h>
#import <UIKit/UIKit.h>

#include "../../src/host/ios_input.h"

namespace {

ios_input_snapshot g_input;
bool g_installed = false;
bool g_menu_open = false;
bool g_controller_connected = false;
} // namespace

@interface VF1TouchButton : UIButton
@property(nonatomic) control_action action;
@end
@implementation VF1TouchButton
@end

@interface VF1TouchOverlay : UIView
@end

static VF1TouchOverlay *g_overlay;

static void update_touch_visibility(bool connected)
{
	if (g_controller_connected != connected) {
		// Do not carry held touch buttons into physical-controller input or back.
		g_input = {};
		g_controller_connected = connected;
	}
	g_overlay.hidden = connected;
	for (VF1TouchButton *button in g_overlay.subviews)
		button.hidden = g_menu_open && button.action != C_MENU;
	if (!connected) [g_overlay.superview bringSubviewToFront:g_overlay];
}

@implementation VF1TouchOverlay
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
	UIView *hit = [super hitTest:point withEvent:event];
	return hit == self ? nil : hit; // Menu gestures reach SDL through unused overlay space.
}

- (VF1TouchButton *)button:(NSString *)title action:(control_action)action frame:(CGRect)frame
{
	for (VF1TouchButton *existing in self.subviews) {
		if (existing.action == action) { existing.frame = frame; return existing; }
	}
	VF1TouchButton *button = [VF1TouchButton buttonWithType:UIButtonTypeSystem];
	button.action = action;
	button.frame = frame;
	button.layer.cornerRadius = frame.size.width * 0.5;
	button.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.35];
	[button setTitle:title forState:UIControlStateNormal];
	[button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
	[button addTarget:self action:@selector(down:) forControlEvents:UIControlEventTouchDown | UIControlEventTouchDragEnter];
	[button addTarget:self action:@selector(up:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel | UIControlEventTouchDragExit];
	[self addSubview:button];
	return button;
}
- (void)down:(VF1TouchButton *)sender
{
	if (sender.action == C_MENU) g_input.menu_pressed = true;
	else g_input.down[0][sender.action] = true;
}
- (void)up:(VF1TouchButton *)sender
{
	if (sender.action != C_MENU) g_input.down[0][sender.action] = false;
}
- (void)layoutSubviews
{
	[super layoutSubviews];
	const CGFloat d = 58.0, gap = 8.0;
	const CGRect b = UIEdgeInsetsInsetRect(self.bounds, self.safeAreaInsets);

	[self button:@"↑" action:C_UP frame:CGRectMake(CGRectGetMinX(b) + d + gap, CGRectGetMaxY(b) - 3*d - 2*gap, d, d)];
	[self button:@"←" action:C_LEFT frame:CGRectMake(CGRectGetMinX(b), CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"↓" action:C_DOWN frame:CGRectMake(CGRectGetMinX(b) + d + gap, CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"→" action:C_RIGHT frame:CGRectMake(CGRectGetMinX(b) + 2*(d + gap), CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"G" action:C_GUARD frame:CGRectMake(CGRectGetMaxX(b) - 3*d - 2*gap, CGRectGetMaxY(b) - 2*d - gap, d, d)];
	[self button:@"P" action:C_PUNCH frame:CGRectMake(CGRectGetMaxX(b) - 2*d - gap, CGRectGetMaxY(b) - 3*d - 2*gap, d, d)];
	[self button:@"K" action:C_KICK frame:CGRectMake(CGRectGetMaxX(b) - 2*d - gap, CGRectGetMaxY(b) - d, d, d)];
	[self button:@"Coin" action:C_COIN frame:CGRectMake(CGRectGetMaxX(b) - 256, CGRectGetMinY(b) + 10, 76, 38)];
	[self button:@"Start" action:C_START frame:CGRectMake(CGRectGetMaxX(b) - 170, CGRectGetMinY(b) + 10, 76, 38)];
	[self button:@"Menu" action:C_MENU frame:CGRectMake(CGRectGetMaxX(b) - 84, CGRectGetMinY(b) + 10, 76, 38)];
}
@end

void ios_input_install(SDL_Window *window)
{
	if (g_installed) return;
	UIWindow *native = (__bridge UIWindow *)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
		SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
	if (!native) return;
	g_installed = true;
	g_overlay = [[VF1TouchOverlay alloc] initWithFrame:native.bounds];
	g_overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	g_overlay.backgroundColor = UIColor.clearColor;
	g_overlay.multipleTouchEnabled = YES;
	[native addSubview:g_overlay];
	[g_overlay setNeedsLayout];
	[g_overlay layoutIfNeeded];
	update_touch_visibility(GCController.controllers.count != 0);
}

void ios_input_reset()
{
	g_input = {};
}

void ios_input_set_menu(bool open)
{
 ios_input_menu_transition(g_input,g_menu_open,open);
 update_touch_visibility(GCController.controllers.count != 0);
}

void ios_input_uninstall()
{
	ios_input_reset();
	[g_overlay removeFromSuperview];
	g_overlay = nil;
	g_installed = false;
	g_menu_open = g_controller_connected = false;
}

ios_input_snapshot ios_input_read()
{
	// UIKit and the SDL iOS host run on the main thread. Polling avoids callback races,
	// overlapping-button releases, stale disconnect state, and duplicate observer ownership.
	NSArray<GCController *> *connected = GCController.controllers;
	update_touch_visibility(connected.count != 0);
	ios_input_snapshot copy = g_input;
	g_input.menu_pressed = false;
	// Physical controller sampling is owned by SDL and cfg.ctl in the host.
	// This bridge merges touch actions only, so remaps and player selection work.
	return copy;
}
