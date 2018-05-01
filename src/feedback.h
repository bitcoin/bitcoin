// Copyright (c) 2016-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FEEDBACK_H
#define FEEDBACK_H
#include "script/script.h"
#include "serialize.h"
enum FeedbackUser {
	FEEDBACKNONE=0,
    FEEDBACKBUYER=1,
	FEEDBACKSELLER=2,
	FEEDBACKARBITER=3
};
class CFeedback {
public:
	std::vector<unsigned char> vchFeedback;
	unsigned char nRating;
	unsigned char nFeedbackUserTo;
	unsigned char nFeedbackUserFrom;
    CFeedback() {
        SetNull();
    }
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(vchFeedback);
		READWRITE(VARINT(nRating));
		READWRITE(VARINT(nFeedbackUserFrom));
		READWRITE(VARINT(nFeedbackUserTo));
	}

    friend bool operator==(const CFeedback &a, const CFeedback &b) {
        return (
        a.vchFeedback == b.vchFeedback
		&& a.nRating == b.nRating
		&& a.nFeedbackUserFrom == b.nFeedbackUserFrom
		&& a.nFeedbackUserTo == b.nFeedbackUserTo
        );
    }

    CFeedback operator=(const CFeedback &b) {
        vchFeedback = b.vchFeedback;
		nRating = b.nRating;
		nFeedbackUserFrom = b.nFeedbackUserFrom;
		nFeedbackUserTo = b.nFeedbackUserTo;
        return *this;
    }

    friend bool operator!=(const CFeedback &a, const CFeedback &b) {
        return !(a == b);
    }
	static std::string FeedbackUserToString(const unsigned char nFeedbackUser) {
		switch (nFeedbackUser) {
			case FEEDBACKNONE:			return "NONE";
			case FEEDBACKBUYER:			return "BUYER";
			case FEEDBACKSELLER:		return "SELLER";
			case FEEDBACKARBITER:		return "ARBITER";
			default:                    return "";
		}
	}
    void SetNull() {  nRating = 0; nFeedbackUserFrom = 0; nFeedbackUserTo = 0; vchFeedback.clear();}
    bool IsNull() const { return (  nRating == 0 && nFeedbackUserFrom == 0 && nFeedbackUserTo == 0 && vchFeedback.empty()); }
};
#endif // FEEDBACK_H