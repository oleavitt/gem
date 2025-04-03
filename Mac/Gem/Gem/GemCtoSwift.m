//
//  GemCtoSwift.m
//  Gem
//
// This is an intermediate wrapper interface for bridging the Gem C code to Swift.
//
// Provides a Swift-friendly Objective-C interface to the C code.
// The C code itself contains a lot of preprocessor macros and functions that take pointers.
// Stuff that does not interface smoothly with Swift.
// So we work it into a cleaner interface by handling the pointers here and exposing a pointerless interface to Swift.
//
//  Created by Oren Leavitt on 11/28/16.
//  Copyright © 2016 Gem. All rights reserved.
//

#import "GemCtoSwift.h"
#include "math3d.h"
#include "image.h"
#include "rend2dc.h"
#include "raytrace.h"
#include "scn20.h"

void Scn20StatusMessage(const char *message)
{
//    int result;
//    LPMSGWNDDATA pmwd = (LPMSGWNDDATA)GetWindowLong(hwndMsgWnd, 0);
//    assert(pmwd != NULL);
//    result = SendMessage(pmwd->hlbwnd, LB_ADDSTRING, 0, (LPARAM)lpszMsg);
//    if (result == LB_ERRSPACE)   /* burp! */
//    {
//        SendMessage(pmwd->hlbwnd, LB_RESETCONTENT, 0, 0);
//        SendMessage(pmwd->hlbwnd, LB_ADDSTRING, 0, (LPARAM)lpszMsg);
//    }
    NSString *msg = [NSString stringWithCString:message encoding:NSASCIIStringEncoding];
    
    NSLog(@"%@", msg);
}
