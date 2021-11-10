//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_BOUNDARY_TYPES_HPP_INCLUDED
#define BOOST_LOCALE_BOUNDARY_TYPES_HPP_INCLUDED

#include <boost/locale/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif


namespace boost {

    namespace locale {
        
        ///
        /// \brief This namespase contains all operations required for boundary analysis of text
        ///
        namespace boundary {
            ///
            /// \defgroup boundary Boundary Analysis
            ///
            /// This module contains all operations required for boundary analysis of text: character, word, like and sentence boundaries
            ///
            /// @{
            ///

            ///
            /// This type describes a possible boundary analysis alternatives.
            ///
            enum boundary_type {
                character,  ///< Analyse the text for character boundaries
                word,       ///< Analyse the text for word boundaries
                sentence,   ///< Analyse the text for Find sentence boundaries
                line        ///< Analyse the text for positions suitable for line breaks
            };

            ///
            /// \brief Flags used with word boundary analysis -- the type of the word, line or sentence boundary found.
            ///
            /// It is a bit-mask that represents various combinations of rules used to select this specific boundary.
            ///
            typedef uint32_t rule_type;

            ///
            /// \anchor bl_boundary_word_rules 
            /// \name Flags that describe a type of word selected
            /// @{
            static const rule_type
                word_none       =  0x0000F,   ///< Not a word, like white space or punctuation mark
                word_number     =  0x000F0,   ///< Word that appear to be a number
                word_letter     =  0x00F00,   ///< Word that contains letters, excluding kana and ideographic characters 
                word_kana       =  0x0F000,   ///< Word that contains kana characters
                word_ideo       =  0xF0000,   ///< Word that contains ideographic characters
                word_any        =  0xFFFF0,   ///< Any word including numbers, 0 is special flag, equivalent to 15
                word_letters    =  0xFFF00,   ///< Any word, excluding numbers but including letters, kana and ideograms.
                word_kana_ideo  =  0xFF000,   ///< Word that includes kana or ideographic characters
                word_mask       =  0xFFFFF;   ///< Full word mask - select all possible variants
            /// @}

            ///
            /// \anchor bl_boundary_line_rules 
            /// \name Flags that describe a type of line break
            /// @{
            static const rule_type 
                line_soft       =  0x0F,   ///< Soft line break: optional but not required
                line_hard       =  0xF0,   ///< Hard line break: like break is required (as per CR/LF)
                line_any        =  0xFF,   ///< Soft or Hard line break
                line_mask       =  0xFF;   ///< Select all types of line breaks
            
            /// @}
            
            ///
            /// \anchor bl_boundary_sentence_rules 
            /// \name Flags that describe a type of sentence break
            ///
            /// @{
            static const rule_type
                sentence_term   =  0x0F,    ///< \brief The sentence was terminated with a sentence terminator 
                                            ///  like ".", "!" possible followed by hard separator like CR, LF, PS
                sentence_sep    =  0xF0,    ///< \brief The sentence does not contain terminator like ".", "!" but ended with hard separator
                                            ///  like CR, LF, PS or end of input.
                sentence_any    =  0xFF,    ///< Either first or second sentence break type;.
                sentence_mask   =  0xFF;    ///< Select all sentence breaking points

            ///@}

            ///
            /// \name  Flags that describe a type of character break.
            ///
            /// At this point break iterator does not distinguish different
            /// kinds of characters so it is used for consistency.
            ///@{
            static const rule_type
                character_any   =  0xF,     ///< Not in use, just for consistency
                character_mask  =  0xF;     ///< Select all character breaking points

            ///@}

            ///
            /// This function returns the mask that covers all variants for specific boundary type
            ///
            inline rule_type boundary_rule(boundary_type t)
            {
                switch(t) {
                case character: return character_mask;
                case word:      return word_mask;
                case sentence:  return sentence_mask;
                case line:      return line_mask;
                default:        return 0;
                }
            }

            ///
            ///@}
            ///

        } // boundary
    } // locale
} // boost
            

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
