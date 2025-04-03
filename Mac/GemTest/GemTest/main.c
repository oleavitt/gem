//
//  main.c
//  GemTest
//
//  Created by Oren Leavitt on 11/27/16.
//  Copyright © 2016 OrenLeavitt. All rights reserved.
//

#include <stdio.h>
#include "math3d.h"
#include "image.h"

int main(int argc, const char * argv[]) {
    Image_Initialize();
    // insert code here...
    Vec3 v1, v2, v3;
    V3Set(&v1, 10, 10, 5);
    V3Copy(&v2, &v1);
    V3Normalize(&v2);
    V3Zero(&v3);
    printf("v1 = <%g, %g, %g>\n", v1.x, v1.y, v1.z);
    printf("v2 = <%g, %g, %g>\n", v2.x, v2.y, v2.z);
    printf("v3 = <%g, %g, %g>\n", v3.x, v3.y, v3.z);
    Image_Close();
    return 0;
}
