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
    int32_t i = 0;
    for (iterOfXLSElement = xlsElementList.begin(); iterOfXLSElement != xlsElementList.end(); iterOfXLSElement++) {
         XLSReader::XLSElement* pXLSElement = *iterOfXLSElement;
         pXLSElement->dump();
         i++;
    }
    printf("Count of XLSElement:%d\n", i);
    return 1;
}
