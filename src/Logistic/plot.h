#pragma once

#include <util/common/plot/plot.h>
#include <util/common/plot/common.h>
#include <set>

namespace plot
{

    class bitmap_drawable : public drawable
    {

    public:

        CBitmap *bitmap;
        bool *visibility;

        bitmap_drawable(CBitmap *bitmap, bool *visibility)
			: bitmap(bitmap)
            , visibility(visibility)
		{
		}

		virtual void draw(CDC &dc, const viewport &bounds) override
		{
            if ((visibility == NULL) || (!*visibility)) return;
            if (bitmap->m_hObject == NULL) return;
            BITMAP bmp; bitmap->GetBitmap(&bmp);
            CDC mdc; mdc.CreateCompatibleDC(&dc);
            CBitmap *old = (CBitmap *)mdc.SelectObject(bitmap);
            dc.StretchBlt(bounds.screen.xmin, bounds.screen.ymin, bounds.screen.width(), bounds.screen.height(), &mdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
            mdc.SelectObject(old);
            mdc.DeleteDC();
		}
    };
}