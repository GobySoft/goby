#include <dccl/codecs3/field_codec_default.h>

#include "waveglider_sv2_codecs.h"
#include "waveglider_sv2_frontseat_driver.pb.h"


extern "C"
{
    void dccl3_load(dccl::Codec* dccl)
    {
        using namespace dccl;
        
        FieldCodecManager::add<goby::moos::SV2IdentifierCodec>("SV2.id");
        FieldCodecManager::add<dccl::v3::DefaultMessageCodec,
                               google::protobuf::FieldDescriptor::TYPE_MESSAGE>("SV2");
        FieldCodecManager::add<dccl::v3::DefaultBytesCodec,
                               google::protobuf::FieldDescriptor::TYPE_BYTES>("SV2");
        FieldCodecManager::add<goby::moos::SV2NumericCodec<dccl::uint32> >("SV2");
        
        dccl->load<goby::moos::protobuf::SV2RequestEnumerate>();
        dccl->load<goby::moos::protobuf::SV2ReplyEnumerate>();
        dccl->load<goby::moos::protobuf::SV2RequestStatus>();
        dccl->load<goby::moos::protobuf::SV2ReplyStatus>();
        dccl->load<goby::moos::protobuf::SV2RequestQueuedMessage>();
        dccl->load<goby::moos::protobuf::SV2ReplyQueuedMessage>();
        dccl->load<goby::moos::protobuf::SV2ACKNAKQueuedMessage>();
        dccl->load<goby::moos::protobuf::SV2GenericNAK>();
        dccl->load<goby::moos::protobuf::SV2GenericACK>();
        dccl->load<goby::moos::protobuf::SV2SendToConsole>();
        dccl->load<goby::moos::protobuf::SV2CommandFollowFixedHeading>();
        dccl->load<goby::moos::protobuf::SV2CommandFollowFixedHeading::CommandFollowFixedHeadingBody>();

    }
    
    void dccl3_unload(dccl::Codec* dccl)
    {
        using namespace dccl;

        dccl->unload<goby::moos::protobuf::SV2RequestEnumerate>();
        dccl->unload<goby::moos::protobuf::SV2ReplyEnumerate>();
        dccl->unload<goby::moos::protobuf::SV2RequestStatus>();
        dccl->unload<goby::moos::protobuf::SV2ReplyStatus>();
        dccl->unload<goby::moos::protobuf::SV2RequestQueuedMessage>();
        dccl->unload<goby::moos::protobuf::SV2ReplyQueuedMessage>();
        dccl->unload<goby::moos::protobuf::SV2ACKNAKQueuedMessage>();
        dccl->unload<goby::moos::protobuf::SV2GenericNAK>();
        dccl->unload<goby::moos::protobuf::SV2GenericACK>();
        dccl->unload<goby::moos::protobuf::SV2SendToConsole>();
        dccl->unload<goby::moos::protobuf::SV2CommandFollowFixedHeading>();
        dccl->unload<goby::moos::protobuf::SV2CommandFollowFixedHeading::CommandFollowFixedHeadingBody>();

        FieldCodecManager::remove<goby::moos::SV2IdentifierCodec>("SV2.id");
        FieldCodecManager::remove<dccl::v3::DefaultMessageCodec,
                                  google::protobuf::FieldDescriptor::TYPE_MESSAGE>("SV2");
        FieldCodecManager::remove<dccl::v3::DefaultBytesCodec,
                                  google::protobuf::FieldDescriptor::TYPE_BYTES>("SV2");
        FieldCodecManager::remove<goby::moos::SV2NumericCodec<dccl::uint32> >("SV2");
                
    }

}
