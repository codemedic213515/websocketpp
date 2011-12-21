/*
 * Copyright (c) 2011, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "data.hpp"

#include "../processors/processor.hpp"

using websocketpp::message::data;

data::data() {
    m_payload.reserve(PAYLOAD_SIZE_INIT);
}
    
websocketpp::frame::opcode::value data::get_opcode() const {
    return m_opcode;
}
	
const std::string& data::get_payload() const {
    return m_payload;
}
	
uint64_t data::process_payload(std::istream& input,uint64_t size) {
    unsigned char c;
    const uint64_t new_size = m_payload.size() + size;
    uint64_t i;
    
    if (new_size > PAYLOAD_SIZE_MAX) {
        // TODO: real exception
        throw processor::exception("Message too big",processor::error::MESSAGE_TOO_BIG);
    }
    
    if (new_size > m_payload.capacity()) {
        m_payload.reserve(std::max(new_size,static_cast<uint64_t>(2*m_payload.capacity())));
    }
    
    for (i = 0; i < size; ++i) {
        if (input.good()) {
            c = input.get();
        } else if (input.eof()) {
            break;
        } else {
            // istream read error? throw?
            throw processor::exception("istream read error",
                                       processor::error::FATAL_ERROR);
        }
        if (input.good()) {
            process_character(c);
        } else if (input.eof()) {
            break;
        } else {
            // istream read error? throw?
            throw processor::exception("istream read error",
                                       processor::error::FATAL_ERROR);
        }
    }
    
    // successfully read all bytes
    return i;
}
	
void data::process_character(unsigned char c) {
    if (m_masking_index >= 0) {
        c = c ^ m_masking_key[(m_masking_index++)%4];
    }
    
    if (m_opcode == frame::opcode::TEXT && !m_validator.consume(static_cast<uint32_t>((unsigned char)(c)))) {
        throw processor::exception("Invalid UTF8 data",processor::error::PAYLOAD_VIOLATION);
    }
    
    // add c to payload 
    m_payload.push_back(c);
}
    
void data::reset(frame::opcode::value opcode) {
    m_opcode = opcode;
    m_masking_index = 0;
    m_payload.resize(0);
    m_validator.reset();
}
		
void data::complete() {
    if (m_opcode == frame::opcode::TEXT) {
        if (!m_validator.complete()) {
            throw processor::exception("Invalid UTF8 data",processor::error::PAYLOAD_VIOLATION);
        }
    }
}
	
void data::set_masking_key(int32_t key) {
    *reinterpret_cast<int32_t*>(m_masking_key) = key;
    m_masking_index = (key == 0 ? -1 : 0);
}