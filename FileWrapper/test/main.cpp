#include "text/TextXLSReader.h"

const std::string fileName="example_text.xls";

int main()
{
    std::list<XLSReader::XLSElement*> xlsElementList;
    if (!TextXLSReader::getElementsFrom(fileName, xlsElementList)) {
        //LOG ERR
        return -1;
    }

    std::list<XLSReader::XLSElement*>::iterator iterOfXLSElement;
    for (iterOfXLSElement = xlsElementList.begin(); iterOfXLSElement != xlsElementList.end(); iterOfXLSElement++) {
         //iterOfXLSElement->dump();
    }
    return 1;
}
