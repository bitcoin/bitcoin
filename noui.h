// Copyright (c) 2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.


inline int MyMessageBox(const string& message, const string& caption="Message", int style=4, void* parent=NULL, int x=-1, int y=-1)
{
    printf("%s: %s\n", caption.c_str(), message.c_str());
    fprintf(stderr, "%s: %s\n", caption.c_str(), message.c_str());
    return 4;
}
#define wxMessageBox  MyMessageBox

inline int ThreadSafeMessageBox(const string& message, const string& caption, int style, void* parent, int x, int y)
{
    return MyMessageBox(message, caption, style, parent, x, y);
}

inline bool ThreadSafeAskFee(int64 nFeeRequired, const string& strCaption, void* parent)
{
    return true;
}

inline void CalledSetStatusBar(const string& strText, int nField)
{
}

inline void UIThreadCall(boost::function0<void> fn)
{
}

inline void MainFrameRepaint()
{
}
