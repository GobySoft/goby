namespace queue
{    
    class QueueKey 
    {
      public:
      QueueKey(QueueType type = queue_notype, unsigned id = 0)
          : type_(type),
            id_(id)
            { }
        void set_type(QueueType type) { type_ = type; }
        void set_id(unsigned id)      { id_ = id; }        

        QueueType type() const { return type_; }
        unsigned id() const { return id_; }        
        
      private:
        QueueType type_;
        unsigned id_;
    };

    inline std::ostream& operator<< (std::ostream& out, const QueueKey& qk)
    { return out << "type: " << qk.type() << " | " << "id: " << qk.id(); }
    
    inline bool operator<(const QueueKey& a, const QueueKey& b)
    { return (a.id() == b.id()) ? a.type() < b.type() : a.id() < b.id(); }

    inline bool operator==(const QueueKey& a, const QueueKey& b)
    { return (a.id() == b.id()) && (a.type() == b.type()); }

}
