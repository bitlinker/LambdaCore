#include <Model.h>
#include <Exception/Exception.h>
#include <Common/StringUtils.h>

using namespace Commons;

namespace LambdaCore
{  
    static const uint32_t CURRENT_MDL_VERSION = 10;

    Model::Model(const Commons::IOStreamPtr stream)
    {
        stream->read(&mHeader, sizeof(Header));

        if (strncmp(mHeader.ID, "IDST", 4))
        {
            throw SerializationException(StringUtils::FormatString("Not a valid model file: %s", mHeader.ID));
        }

        if (mHeader.Version != CURRENT_MDL_VERSION)
        {
            throw SerializationException(StringUtils::FormatString("Unsupported model version: %d", mHeader.Version));
        }


        int k = 0;
    }
}