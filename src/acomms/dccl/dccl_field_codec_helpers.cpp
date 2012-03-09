
#include "dccl_field_codec_helpers.h"
#include "dccl_field_codec.h"

std::vector<const google::protobuf::FieldDescriptor*> goby::acomms::MessageHandler::field_;
std::vector<const google::protobuf::Descriptor*> goby::acomms::MessageHandler::desc_;


//
// BitsHandler
//
goby::acomms::BitsHandler::BitsHandler(Bitset* out_pool, Bitset* in_pool, bool lsb_first /*= true*/)
    : lsb_first_(lsb_first),
      in_pool_(in_pool),
      out_pool_(out_pool)
{
    connection_ = DCCLFieldCodecBase::get_more_bits.connect(
        boost::bind(&BitsHandler::transfer_bits, this, _1),
        boost::signals::at_back);
}
                
void goby::acomms::BitsHandler::transfer_bits(unsigned size)
{
    glog.is(common::logger::DEBUG3) && glog  <<  "_get_bits from (" << in_pool_ << ") " << *in_pool_ << " to add to (" << out_pool_ << ") " << *out_pool_ << " number: " << size << std::endl;
    
    if(lsb_first_)
    {
        // grab lowest bits first
        for(int i = 0, n = size; i < n; ++i)
            out_pool_->push_back((*in_pool_)[i]);
        *in_pool_ >>= size;
    }
    else
    {
        // grab highest bits first
        out_pool_->resize(out_pool_->size() + size);
        *out_pool_ <<= size;
        for(int i = 0, n = size; i < n; ++i)
        {
            (*out_pool_)[size-i-1] = (*in_pool_)[in_pool_->size()-i-1];
        }
        
    }
    in_pool_->resize(in_pool_->size()-size);
}



//
// MessageHandler
//

void goby::acomms::MessageHandler::push(const google::protobuf::Descriptor* desc)
 
{
    desc_.push_back(desc);
    ++descriptors_pushed_;
}

void goby::acomms::MessageHandler::push(const google::protobuf::FieldDescriptor* field)
{
    field_.push_back(field);
    ++fields_pushed_;
}


void goby::acomms::MessageHandler::__pop_desc()
{
    if(!desc_.empty())
        desc_.pop_back();
}

void goby::acomms::MessageHandler::__pop_field()
{
    if(!field_.empty())
        field_.pop_back();
}

goby::acomms::MessageHandler::MessageHandler(const google::protobuf::FieldDescriptor* field)
    : descriptors_pushed_(0),
      fields_pushed_(0)
{
    if(field)
    {
        if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            push(field->message_type());
        push(field);
    }
}

