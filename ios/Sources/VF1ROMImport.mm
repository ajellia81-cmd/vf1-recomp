#import "VF1ROMImport.h"
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include "../../src/host/romset.h"

@interface VF1ROMImport () <UIDocumentPickerDelegate>
@property(nonatomic, copy) void (^completion)(NSURL *, NSString *);
@property(nonatomic, strong) UIView *progressView;
@end

@implementation VF1ROMImport

+ (NSURL *)sandboxROMURL
{
	NSURL *support = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
		inDomains:NSUserDomainMask].firstObject;
	NSURL *appFolder = [support URLByAppendingPathComponent:@"Virtua Fighter Recomp" isDirectory:YES];
	NSURL *romFolder = [appFolder URLByAppendingPathComponent:@"roms" isDirectory:YES];
	return [romFolder URLByAppendingPathComponent:@"vf.zip"];
}

+ (void)presentFrom:(UIViewController *)presenter completion:(void (^)(NSURL *, NSString *))completion
{
	VF1ROMImport *delegate = [VF1ROMImport new];
	delegate.completion = completion;
	UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc]
		initForOpeningContentTypes:@[UTTypeData] asCopy:YES];
	// Providers sometimes report ZIP archives as generic data. Validate contents,
	// not the provider's type label. Import a copy, never open the external file in place.
	picker.allowsMultipleSelection = NO;
	picker.delegate = delegate;
	picker.modalPresentationStyle = UIModalPresentationFormSheet;
	// The presented controller retains its delegate for the selection lifetime.
	objc_setAssociatedObject(picker, @selector(presentFrom:completion:), delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[presenter presentViewController:picker animated:YES completion:nil];
}

- (void)finishWithURL:(NSURL *)url error:(NSString *)error
{
	// All UIKit and the host's completion state stay on the main thread.
	dispatch_async(dispatch_get_main_queue(), ^{
		[self.progressView removeFromSuperview];
		self.progressView = nil;
		void (^completion)(NSURL *, NSString *) = self.completion;
		self.completion = nil;
		if (completion) completion(url, error);
	});
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
	NSURL *source = urls.firstObject;
	if (!source) {
		[self finishWithURL:nil error:@"The selected archive could not be accessed."];
		return;
	}
	UIWindow *window = controller.view.window ?: controller.presentingViewController.view.window;
	UIView *progress = [[UIView alloc] initWithFrame:window.bounds];
	progress.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	progress.backgroundColor = UIColor.systemBackgroundColor;
	UIActivityIndicatorView *spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
	spinner.translatesAutoresizingMaskIntoConstraints = NO;
	[spinner startAnimating];
	UILabel *label = [UILabel new];
	label.text = @"Copying and checking your archive…";
	label.translatesAutoresizingMaskIntoConstraints = NO;
	[progress addSubview:spinner];
	[progress addSubview:label];
	[NSLayoutConstraint activateConstraints:@[
		[spinner.centerXAnchor constraintEqualToAnchor:progress.centerXAnchor],
		[spinner.centerYAnchor constraintEqualToAnchor:progress.centerYAnchor constant:-20],
		[label.centerXAnchor constraintEqualToAnchor:progress.centerXAnchor],
		[label.topAnchor constraintEqualToAnchor:spinner.bottomAnchor constant:16]
	]];
	[window addSubview:progress];
	self.progressView = progress;
	// File coordination may wait for an iCloud/provider download; CRC checking
	// also does substantial I/O. Neither may block UIKit's main run loop.
	dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
		@autoreleasepool {
			const BOOL scoped = [source startAccessingSecurityScopedResource];
			__block NSError *copyError = nil;
			NSError *coordinationError = nil;
			__block NSURL *temporary = nil;
			NSFileCoordinator *coordinator = [NSFileCoordinator new];
			[coordinator coordinateReadingItemAtURL:source options:0 error:&coordinationError byAccessor:^(NSURL *readURL) {
				NSURL *destination = [VF1ROMImport sandboxROMURL];
				[[NSFileManager defaultManager] createDirectoryAtURL:[destination URLByDeletingLastPathComponent]
					withIntermediateDirectories:YES attributes:nil error:&copyError];
				if (copyError) return;
				temporary = [[destination URLByDeletingLastPathComponent] URLByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".zip"]];
				[[NSFileManager defaultManager] removeItemAtURL:temporary error:nil];
				[[NSFileManager defaultManager] copyItemAtURL:readURL toURL:temporary error:&copyError];
			}];
			if (scoped) [source stopAccessingSecurityScopedResource];
			if (coordinationError && !copyError) copyError = coordinationError;
			if (copyError || !temporary) {
				if (temporary) [[NSFileManager defaultManager] removeItemAtURL:temporary error:nil];
				[self finishWithURL:nil error:copyError.localizedDescription ?: @"The archive could not be copied."];
				return;
			}
			romset::result checked = romset::load({temporary.path.UTF8String});
			if (!checked.ok) {
				[[NSFileManager defaultManager] removeItemAtURL:temporary error:nil];
				[self finishWithURL:nil error:[NSString stringWithUTF8String:checked.error.c_str()]];
				return;
			}
			NSURL *destination = [VF1ROMImport sandboxROMURL];
			NSFileManager *manager = NSFileManager.defaultManager;
			BOOL installed;
			if ([manager fileExistsAtPath:destination.path])
				installed = [manager replaceItemAtURL:destination withItemAtURL:temporary backupItemName:nil
					options:0 resultingItemURL:nil error:&copyError];
			else installed = [manager moveItemAtURL:temporary toURL:destination error:&copyError];
			if (!installed) {
				[manager removeItemAtURL:temporary error:nil];
				[self finishWithURL:nil error:copyError.localizedDescription ?: @"The archive could not be installed."];
				return;
			}
			[self finishWithURL:destination error:nil];
		}
	});
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
	[self finishWithURL:nil error:nil];
}
@end
