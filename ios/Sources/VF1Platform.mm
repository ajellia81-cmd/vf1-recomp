#import <UIKit/UIKit.h>
#import "VF1ROMImport.h"
#include "../../src/host/ios_platform.h"
#include <SDL3/SDL.h>

// UIDocumentPicker dismisses itself. Only dismiss a controller that is still
// present, and wait for an active dismissal before showing an error/retrying.
static void after_import_modal(UIViewController *presenter, dispatch_block_t completion)
{
	UIViewController *modal = presenter.presentedViewController;
	if (!modal) { completion(); return; }
	if (modal.isBeingDismissed) {
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
			after_import_modal(presenter, completion);
		});
		return;
	}
	[presenter dismissViewControllerAnimated:NO completion:completion];
}

std::string ios_data_directory()
{
	NSURL *url = [[VF1ROMImport sandboxROMURL] URLByDeletingLastPathComponent];
	return url.URLByDeletingLastPathComponent.path.UTF8String;
}

std::string ios_import_rom()
{
	if (!SDL_Init(SDL_INIT_VIDEO)) return {};
	SDL_Window *window = SDL_CreateWindow("Import your VF1 archive", 960, 540, SDL_WINDOW_FULLSCREEN);
	if (!window) return {};
	UIWindow *native = (__bridge UIWindow *)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
		SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
	UIViewController *presenter = native.rootViewController;
	if (!presenter) {
		SDL_SetError("SDL's UIKit presenter is unavailable");
		SDL_DestroyWindow(window);
		return {};
	}
	std::string result;
	bool retry = true;
	while (retry) {
		__block bool done = false;
		__block bool again = false;
		__block std::string selected;
		[VF1ROMImport presentFrom:presenter completion:^(NSURL *url, NSString *error) {
			if (url) selected = url.path.UTF8String;
			if (error) SDL_SetError("%s", error.UTF8String);
			after_import_modal(presenter, ^{
				if (!error) { done = true; return; }
				UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Unable to import archive"
					message:error preferredStyle:UIAlertControllerStyleAlert];
				[alert addAction:[UIAlertAction actionWithTitle:@"Choose another file" style:UIAlertActionStyleDefault
					handler:^(UIAlertAction *) { again = true; after_import_modal(presenter, ^{ done = true; }); }]];
				[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel
					handler:^(UIAlertAction *) { after_import_modal(presenter, ^{ done = true; }); }]];
				[presenter presentViewController:alert animated:YES completion:nil];
			});
		}];
		// Service UIKit and main-queue import completions while the host waits.
		// Keep the window and block state alive until import/error handling is complete.
		while (!done) {
			SDL_PumpEvents();
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, true);
		}
		result = selected;
		retry = again;
	}

	SDL_DestroyWindow(window);
	return result;
}

void ios_safe_area(SDL_Window *window, float &left, float &top, float &right, float &bottom)
{
	UIWindow *native = (__bridge UIWindow *)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
		SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
	const UIEdgeInsets insets = native.safeAreaInsets;
	const float density = SDL_GetWindowPixelDensity(window);
	left = insets.left * density; top = insets.top * density;
	right = insets.right * density; bottom = insets.bottom * density;
}
