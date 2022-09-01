#pragma once

#include "rtti.hpp"
#include <Geode/Geode.hpp>

USE_GEODE_NAMESPACE();

class GDRTTI : public RTTI {
    protected:
        GDRTTI();
        ~GDRTTI();

    public:
        static GDRTTI& get();
        rtti_t read_rtti(void*);
        bool valid(uintptr_t pointer);

        GDRTTI(GDRTTI const&) = delete;
	    GDRTTI& operator=( const GDRTTI& ) = delete;
};
