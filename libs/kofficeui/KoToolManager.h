/* This file is part of the KDE project
 *  Copyright (c) 2005-2006 Boudewijn Rempt <boud@valdyas.org>
 * Copyright (C) 2006 Thomas Zander <zander@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef KO_TOOL_MANAGER
#define KO_TOOL_MANAGER

#include <koffice_export.h>
#include <QMap>
#include <QObject>
#include <QCursor>
#include <QStack>

#include <KoToolProxy.h>

class ToolHelper;
class KoCanvasController;
class KoCanvasBase;
class KoTool;
class KoShapeControllerBase;
class KoToolBox;
class KoCreateShapesTool;
class KActionCollection;
class KoShape;


/**
 * This class manages the activation and deactivation of tools for
 * each input device.
 *
 * Managing the active tool and switching tool based on various variables.
 *
 * The state of the toolbox will be the same for all views in the process so practically
 * you can say we have one toolbox per application instance (process).  Implementation
 * does not allow one widget to be in more then one view, so we just make sure the toolbox
 * is hidden in not-in-focus views.
 *
 * The ToolManager is a singleton and will manage all views in all applications that
 * are loaded in this process.  his means you will have to register and unregister your view.
 * When creating your new view you should use a KoCanvasController() and register that
 * with the ToolManager like this:
@code
    MyGuiWidget::MyGuiWidget() {
        m_canvasController = new KoCanvasController(this);
        m_canvasController->setCanvas(m_canvas);
        KoToolManager::instance()->addControllers(m_canvasController, myShapeController));
    }
    MyGuiWidget::~MyGuiWidget() {
        KoToolManager::instance()->removeCanvasController(m_canvasController);
    }
@endcode
 *
 * For a new view that extends KoView all you need to do is implement KoView::createToolBox()
 *
 * KoToolManager also keeps track of the current tool based on a
   complex set of conditions and heuristics:

   - there is one active tool per canvas
   - for every pointing device (determined by the unique id of tablet,
     or 0 for mice -- you may have more than one mouse attached, but
     Qt cannot distinquish between them, there is an associated too.
   - depending on things like tablet leave/enter proximity, incoming
     mouse or tablet events and a little timer (that gets stopped when
     we know what is what), the active pointing device is determined,
     and the active tool is set accordingly.

   Nota bene: if you use KoToolManager and register your canvases with
   it you no longer have to manually implement methods to route mouse,
   tablet, key or wheel events to the active tool. In fact, it's no
   longer interesting to you which tool is active; you can safely
   route the paint event through KoToolManager::paint().


   (The reason the input events are handled completely by the
   toolmanager and the paint events not is that, generally speaking,
   it's okay if the tools get the input events first, but you want to
   paint your shapes or other canvas stuff first and only then paint
   the tool stuff.)

 */
class KOFFICEUI_EXPORT KoToolManager : public QObject, public KoToolProxy {
    Q_OBJECT

public:
    /// Return the toolmanager singleton
    static KoToolManager* instance();
    ~KoToolManager();


public: // KoToolProxy implementation

    KoToolProxy * toolProxy() { return this; }

private:

    void paint( QPainter &painter, KoViewConverter &converter );
    void repaintDecorations();
    void tabletEvent( QTabletEvent *event, const QPointF &pnt );
    void mousePressEvent( QMouseEvent *event, const QPointF &pnt );
    void mouseDoubleClickEvent( QMouseEvent *event, const QPointF &pnt );
    void mouseMoveEvent( QMouseEvent *event, const QPointF &pnt );
    void mouseReleaseEvent( QMouseEvent *event, const QPointF &pnt );
    void keyPressEvent(QKeyEvent *event );
    void keyReleaseEvent(QKeyEvent *event);
    void wheelEvent ( QWheelEvent * event, const QPointF &pnt );

public:
    /**
     * Create a new ToolBox with title.
     * This creates a new toolbox that is initialized with all tools registered for you
     * to attach to the view of your application.
     * If your view extends KoView the line of code is:
@code
    shell()->addDockWidget(Qt::LeftDockWidgetArea,
        KoToolManager::instance()->toolBox("MyApp"));
@endcode
     * @param applicationName the title for the toolbox
     */
    KoToolBox *toolBox(const QString &applicationName = QString());

    void registerTools(KActionCollection *ac);

    /**
     * Register a new pair of view controllers
     * @param controller the view controller that this toolmanager will manager the tools for
     * @param sc the shape controller instance that is associated with the controller and which
     *      will be used for things like registring new shapes if a tool creates one.
     */
    void addControllers(KoCanvasController *controller, KoShapeControllerBase *sc);

    /**
     * Remove a set of controllers
     * When the controller is no longer used it should be removed so all tools can be
     * deleted and stop eating memory.  The accompanying KoShapeControllerBase will
     * no longer be referenced afterwards.
     * @param controller the controller that is removed
     */
    void removeCanvasController(KoCanvasController *controller);

    /// @return the active canvas controller
    KoCanvasController *activeCanvasController() const;

    /**
     * Return the tool that is able to create shapes for this param canvas.
     * This is typically used by the KoShapeSelector to set which shape to create next.
     * @param canvas the canvas that is a child of a previously registered controller
     *    who's tool you want.
     * @see addControllers()
     */
    KoCreateShapesTool *shapeCreatorTool(KoCanvasBase *canvas) const;

signals:
    /**
     * Emitted when a new tool was selected or became active.
     * @param uniqueToolId a random but unique code for the new tool.
     */
    void changedTool(int uniqueToolId);

    /**
     * Emitted after the selection changed to state which unique shape-types are now
     * in the selection.
     * @param types a list of string that are the shape types of the selected objects.
     */
    void toolCodesSelected(QList<QString> types);

private:

    KoToolManager();
    KoToolManager(const KoToolManager&);
    KoToolManager operator=(const KoToolManager&);
    void setup();
    void switchTool(KoTool *tool);
    void switchTool(const QString &id, bool temporary);

private slots:

    void toolActivated(ToolHelper *tool);
    void detachCanvas(KoCanvasController *controller);
    void attachCanvas(KoCanvasController *controller);
    void movedFocus(QWidget *from, QWidget *to);
    void updateCursor(QCursor cursor);
    void switchToolRequested(const QString &id);
    void switchToolTemporaryRequested(const QString &id);
    void switchBackRequested();
    void selectionChanged(QList<KoShape*> shapes);

private:

    static KoToolManager* s_instance;

    QList<ToolHelper*> m_tools;
    QMap<KoCanvasController*, KoShapeControllerBase*> m_shapeControllers;
    QList<KoCanvasController*> m_canvases;
    KoCanvasController *m_activeCanvas;
    KoTool *m_activeTool, *m_dummyTool;
    ToolHelper *m_defaultTool; // the pointer thingy
    QString m_activeToolId;

    QMap<KoTool*, int> m_uniqueToolIds; // for the changedTool signal
    QMap<KoCanvasController*, QMap<QString, KoTool*> > m_allTools;
    QStack<QString> m_stack; // stack of temporary tools

    QList<KoShape*> m_lastSelectedShapes;
};

#endif
