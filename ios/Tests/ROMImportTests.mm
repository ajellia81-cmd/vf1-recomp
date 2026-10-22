#import <XCTest/XCTest.h>
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import "../Sources/VF1ROMImport.h"

@interface VF1ROMImport (TestBoundary)
@property(nonatomic, copy) void (^completion)(NSURL *, NSString *);
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls;
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller;
@end

@interface ROMImportTests : XCTestCase
@end
@implementation ROMImportTests
- (void)testInvalidDocumentReturnsBeforeValidationAndCompletesOnMainThread
{
	XCTAssertTrue(NSThread.isMainThread);
	NSURL *directory = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString]];
	[NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
	NSURL *source = [directory URLByAppendingPathComponent:@"readme.txt"];
	[@"Plain text document for an import error-path test; no game data." writeToURL:source atomically:YES encoding:NSUTF8StringEncoding error:nil];
	VF1ROMImport *importer = [VF1ROMImport new];
	XCTestExpectation *finished = [self expectationWithDescription:@"Invalid document is reported"];
	__block BOOL returned = NO;
	importer.completion = ^(NSURL *url, NSString *error) {
		XCTAssertTrue(returned, @"Document I/O must not block the picker delegate");
		XCTAssertTrue(NSThread.isMainThread);
		XCTAssertNil(url);
		XCTAssertTrue([error containsString:@"as a zip file"]);
		XCTAssertTrue([NSFileManager.defaultManager fileExistsAtPath:source.path]);
		[finished fulfill];
	};
	UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeData] asCopy:YES];
	[importer documentPicker:picker didPickDocumentsAtURLs:@[source]];
	returned = YES;
	[self waitForExpectations:@[finished] timeout:15];
	XCTAssertNil(importer.completion);
	[NSFileManager.defaultManager removeItemAtURL:directory error:nil];
}
- (void)testCancellationCompletesOnce
{
	VF1ROMImport *importer = [VF1ROMImport new];
	XCTestExpectation *finished = [self expectationWithDescription:@"Cancel completes once"];
	finished.assertForOverFulfill = YES;
	importer.completion = ^(NSURL *url, NSString *error) {
		XCTAssertTrue(NSThread.isMainThread);
		XCTAssertNil(url);
		XCTAssertNil(error);
		[finished fulfill];
	};
	[importer documentPickerWasCancelled:nil];
	[importer documentPickerWasCancelled:nil];
	[self waitForExpectations:@[finished] timeout:5];
	XCTAssertNil(importer.completion);
}
@end
