//
//  ViewController.m
//  GemRay
//
//  Created by Oren Leavitt on 11/27/16.
//  Copyright © 2016 Oren Leavitt. All rights reserved.
//

#import "ViewController.h"
#import "math3d.h"

@interface ViewController()
@property (unsafe_unretained) IBOutlet NSTextView *textViewOutput;
@property (weak) IBOutlet NSTextField *labelFilePath;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do any additional setup after loading the view.
    [self generateTestOutput];
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}

- (void) generateTestOutput
{
    Vec3 v1, v2, v3;
    V3Set(&v1, 1, 2, 3);
    V3Copy(&v2, &v1);
    V3Normalize(&v2);
    V3Zero(&v3);
    [_textViewOutput setString:[NSString stringWithFormat:@"v1 = <%g, %g, %g>, v2 = <%g, %g, %g>, v3 = <%g, %g, %g>", v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z]];
}

@end
