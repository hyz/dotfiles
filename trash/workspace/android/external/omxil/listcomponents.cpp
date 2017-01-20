#include <stdio.h>
#include <stdlib.h>

#include <OpenMAX/IL/OMX_Core.h>
#include <OpenMAX/IL/OMX_Component.h>
#include <OpenMAX/IL/OMX_Types.h>
#include "frameworks/av/media/libstagefright/omx/OMXMaster.h"

int main(int argc, char** argv)
{
    char name[OMX_MAX_STRINGNAME_SIZE];

    android::OMXMaster* master = new android::OMXMaster();
    OMX_ERRORTYPE err;

    for (int i = 0; OMX_ErrorNoMore != err; i++) {
        err = master->enumerateComponents(name,OMX_MAX_STRINGNAME_SIZE,i);

        if (OMX_ErrorNone == err) {
            android::Vector<android::String8> roles;
            err = master->getRolesOfComponent(name, &roles);

            if (err != OMX_ErrorNone) {
                fprintf(stderr, "Getting roles failed\n");
                exit(1);
            }

            printf("Component is %s\n", name);
            for (int n = 0; n < (int)roles.size(); n++) {
                printf("\trole: %s\n", roles[n].string());
            }
        }
    }

    return(0);
}

