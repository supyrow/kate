/*
    Copyright (C) 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    Copyright (C) 2019 Christoph Cullmann <cullmann@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LSPCLIENTHOVER_H
#define LSPCLIENTHOVER_H

#include "lspclientserver.h"
#include "lspclientservermanager.h"

#include <KTextEditor/TextHintInterface>

class LSPClientHover : public QObject, public KTextEditor::TextHintProvider
{
    Q_OBJECT

public:

    // implementation factory method
    static LSPClientHover* new_(QSharedPointer<LSPClientServerManager> manager);

    LSPClientHover()
        : KTextEditor::TextHintProvider()
    {}

    virtual void setServer(QSharedPointer<LSPClientServer> server) = 0;
};

#endif
