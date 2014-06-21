
#include <qt_windows.h>
#include <ActiveQt>
#include <QAxFactory>
#include "shelloverlay.h"

QT_USE_NAMESPACE

QAXFACTORY_DEFAULT(ShellOverlayBinder,
				   "{4A80CBB7-F51D-41AF-AE6E-F76801B3BFB7}", /* Class ID (CLSID) */
				   "{22D8E702-2C02-4B15-9F11-51F435000288}", /* Interface ID (IID) */
				   "{3EAE68EA-1249-4765-BD67-9D0108CEDA9D}", /* event interface ID */
				   "{93582F04-9B48-4136-93E7-B5D62DD74050}", /* Type Library ID (TLB) */
				   "{F0B2C053-A6C7-4486-B883-3FCB7F53AD37}" /* Application ID (AppID) */
)
