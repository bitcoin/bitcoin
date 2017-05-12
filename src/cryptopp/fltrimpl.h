#ifndef CRYPTOPP_FLTRIMPL_H
#define CRYPTOPP_FLTRIMPL_H

#if CRYPTOPP_MSC_VERSION
# pragma warning(push)
# pragma warning(disable: 4100)
#endif

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-value"
#endif

#define FILTER_BEGIN	\
	switch (m_continueAt)	\
	{	\
	case 0:	\
		m_inputPosition = 0;

#define FILTER_END_NO_MESSAGE_END_NO_RETURN	\
		break;	\
	default:	\
		CRYPTOPP_ASSERT(false);	\
	}

#define FILTER_END_NO_MESSAGE_END	\
	FILTER_END_NO_MESSAGE_END_NO_RETURN	\
	return 0;

/*
#define FILTER_END	\
	case -1:	\
		if (messageEnd && Output(-1, NULL, 0, messageEnd, blocking))	\
			return 1;	\
	FILTER_END_NO_MESSAGE_END
*/

#define FILTER_OUTPUT3(site, statement, output, length, messageEnd, channel)	\
	{\
	case site:	\
	statement;	\
	if (Output(site, output, length, messageEnd, blocking, channel))	\
		return STDMAX(size_t(1), length-m_inputPosition);\
	}

#define FILTER_OUTPUT2(site, statement, output, length, messageEnd)	\
	FILTER_OUTPUT3(site, statement, output, length, messageEnd, DEFAULT_CHANNEL)

#define FILTER_OUTPUT(site, output, length, messageEnd)	\
	FILTER_OUTPUT2(site, 0, output, length, messageEnd)

#define FILTER_OUTPUT_BYTE(site, output)	\
	FILTER_OUTPUT(site, &(const byte &)(byte)output, 1, 0)

#define FILTER_OUTPUT2_MODIFIABLE(site, statement, output, length, messageEnd)	\
	{\
	case site:	\
	statement;	\
	if (OutputModifiable(site, output, length, messageEnd, blocking))	\
		return STDMAX(size_t(1), length-m_inputPosition);\
	}

#define FILTER_OUTPUT_MODIFIABLE(site, output, length, messageEnd)	\
	FILTER_OUTPUT2_MODIFIABLE(site, 0, output, length, messageEnd)

#define FILTER_OUTPUT2_MAYBE_MODIFIABLE(site, statement, output, length, messageEnd, modifiable)	\
	{\
	case site:	\
	statement;	\
	if (modifiable ? OutputModifiable(site, output, length, messageEnd, blocking) : Output(site, output, length, messageEnd, blocking))	\
		return STDMAX(size_t(1), length-m_inputPosition);\
	}

#define FILTER_OUTPUT_MAYBE_MODIFIABLE(site, output, length, messageEnd, modifiable)	\
	FILTER_OUTPUT2_MAYBE_MODIFIABLE(site, 0, output, length, messageEnd, modifiable)

#if CRYPTOPP_MSC_VERSION
# pragma warning(pop)
#endif

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic pop
#endif

#endif
