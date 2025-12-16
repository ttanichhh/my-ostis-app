#pragma once
#include <sc-memory/sc_keynodes.hpp>

class SetProcessingKeynodes : public ScKeynodes
{
public:
    static inline ScKeynode const action_check_graph_features{
        "action_check_graph_features",
        ScType::ConstNodeClass
    };
    static inline ScKeynode const action_check_graph_isomorphism{
        "action_check_graph_isomorphism",
        ScType::ConstNodeClass
    };

    static inline ScKeynode const nrel_work_high_connection{
        "nrel_work_high_connection",
        ScType::ConstNodeNonRole
    };
    static inline ScKeynode const nrel_work_connection{
        "nrel_work_connection",
        ScType::ConstNodeNonRole
    };

    //Возможно, сюда позже надо будет добавить 
    //static inline ScKeynode const rrel_1{"rrel_1", ScType::ConstNodeRole};
    //static inline ScKeynode const rrel_2{"rrel_2", ScType::ConstNodeRole};
    //static inline ScKeynode const rrel_3{"rrel_3", ScType::ConstNodeRole};

    
    static inline ScKeynode const concept_directors{
        "concept_directors", ScType::ConstNodeClass
    };
    static inline ScKeynode const concept_specialists{
        "concept_specialists", ScType::ConstNodeClass
    };
    static inline ScKeynode const concept_workers{
        "concept_workers", ScType::ConstNodeClass
    };

};
