#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pln.hxx>

#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TColgp_Array1OfPnt2d.hxx>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeFillet2d.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>

#include <BRepOffsetAPI_ThruSections.hxx>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>

#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>

#include <QMessageBox>
#include <QDebug>

#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools_ReShape.hxx>

#define MAX2(X, Y)      (  Abs(X) > Abs(Y)? Abs(X) : Abs(Y) )
#define MAX3(X, Y, Z)   ( MAX2 ( MAX2(X,Y) , Z) )

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // occ modeler.
    InitializeModeler();

    occView = new OccView(mContext, this);
    this->setCentralWidget(occView);

    this->resize(this->width()+15, this->height()+15);
    this->createActions();
    this->createMenus();
    this->createToolBars();

    //Single shot to force timer redraw the mainwindow and uptdate occView to resize
    //FIXME:This is a bug to fix
    QTimer::singleShot(100, this, SLOT(timerRedraw()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::InitializeModeler()
{
    Handle_Aspect_DisplayConnection aDisplayConnection;
    Handle_OpenGl_GraphicDriver aGraphicDriver;

    // 1. Create a 3D viewer.
    try
    {
        aDisplayConnection = new Aspect_DisplayConnection (qgetenv ("DISPLAY").constData());
        aGraphicDriver = new OpenGl_GraphicDriver(aDisplayConnection);
    }
    catch (Standard_Failure)
    {
        QMessageBox::critical(this, tr("About occQt"),
                              tr("<h2>Fatal error in graphic initialisation!</h2>"),
                              QMessageBox::Apply);
    }

    mViewer = new V3d_Viewer(aGraphicDriver, Standard_ExtString("Visu3D"));
    mViewer->SetDefaultLights();
    mViewer->SetLightOn();

    // 3. Create an interactive context.
    mContext = new AIS_InteractiveContext(mViewer);
    mContext->SetDisplayMode(AIS_Shaded);
}

void MainWindow::createActions()
{
    mExitAction = new QAction(tr("Exit"), this);
    mExitAction->setShortcut(tr("Ctrl+Q"));
    mExitAction->setIcon(QIcon(":/Resources/close.png"));
    mExitAction->setStatusTip(tr("Exit the application"));
    connect(mExitAction, SIGNAL(triggered()), this, SLOT(close()));

    mViewZoomAction = new QAction(tr("Zoom"), this);
    mViewZoomAction->setIcon(QIcon(":/Resources/Zoom.png"));
    mViewZoomAction->setStatusTip(tr("Zoom the view"));
    connect(mViewZoomAction, SIGNAL(triggered()), occView, SLOT(zoom()));

    mViewPanAction = new QAction(tr("Pan"), this);
    mViewPanAction->setIcon(QIcon(":/Resources/Pan.png"));
    mViewPanAction->setStatusTip(tr("Pan the view"));
    connect(mViewPanAction, SIGNAL(triggered()), occView, SLOT(pan()));

    mViewRotateAction = new QAction(tr("Rotate"), this);
    mViewRotateAction->setIcon(QIcon(":/Resources/Rotate.png"));
    mViewRotateAction->setStatusTip(tr("Rotate the view"));
    connect(mViewRotateAction, SIGNAL(triggered()), occView, SLOT(rotate()));

    mViewResetAction = new QAction(tr("Reset"), this);
    mViewResetAction->setIcon(QIcon(":/Resources/Home.png"));
    mViewResetAction->setStatusTip(tr("Reset the view"));
    connect(mViewResetAction, SIGNAL(triggered()), occView, SLOT(reset()));

    mViewFitallAction = new QAction(tr("Fit All"), this);
    mViewFitallAction->setIcon(QIcon(":/Resources/FitAll.png"));
    mViewFitallAction->setStatusTip(tr("Fit all "));
    connect(mViewFitallAction, SIGNAL(triggered()), occView, SLOT(fitAll()));

    mMakeBoxAction = new QAction(tr("Box"), this);
    mMakeBoxAction->setIcon(QIcon(":/Resources/box.png"));
    mMakeBoxAction->setStatusTip(tr("Make a box"));
    connect(mMakeBoxAction, SIGNAL(triggered()), this, SLOT(makeBox()));

    mMakeConeAction = new QAction(tr("Cone"), this);
    mMakeConeAction->setIcon(QIcon(":/Resources/cone.png"));
    mMakeConeAction->setStatusTip(tr("Make a cone"));
    connect(mMakeConeAction, SIGNAL(triggered()), this, SLOT(makeCone()));

    mMakeSphereAction = new QAction(tr("Sphere"), this);
    mMakeSphereAction->setStatusTip(tr("Make a sphere"));
    mMakeSphereAction->setIcon(QIcon(":/Resources/sphere.png"));
    connect(mMakeSphereAction, SIGNAL(triggered()), this, SLOT(makeSphere()));

    mMakeCylinderAction = new QAction(tr("Cylinder"), this);
    mMakeCylinderAction->setStatusTip(tr("Make a cylinder"));
    mMakeCylinderAction->setIcon(QIcon(":/Resources/cylinder.png"));
    connect(mMakeCylinderAction, SIGNAL(triggered()), this, SLOT(makeCylinder()));

    mMakeTorusAction = new QAction(tr("Torus"), this);
    mMakeTorusAction->setStatusTip(tr("Make a torus"));
    mMakeTorusAction->setIcon(QIcon(":/Resources/torus.png"));
    connect(mMakeTorusAction, SIGNAL(triggered()), this, SLOT(makeTorus()));

    mFilletAction = new QAction(tr("Fillet"), this);
    mFilletAction->setIcon(QIcon(":/Resources/fillet.png"));
    mFilletAction->setStatusTip(tr("Test Fillet algorithm"));
    connect(mFilletAction, SIGNAL(triggered()), this, SLOT(makeFillet()));

    mChamferAction = new QAction(tr("Chamfer"), this);
    mChamferAction->setIcon(QIcon(":/Resources/chamfer.png"));
    mChamferAction->setStatusTip(tr("Test chamfer algorithm"));
    connect(mChamferAction, SIGNAL(triggered()), this, SLOT(makeChamfer()));

    mExtrudeAction = new QAction(tr("Extrude"), this);
    mExtrudeAction->setIcon(QIcon(":/Resources/extrude.png"));
    mExtrudeAction->setStatusTip(tr("Test extrude algorithm"));
    connect(mExtrudeAction, SIGNAL(triggered()), this, SLOT(makeExtrude()));

    mRevolveAction = new QAction(tr("Revolve"), this);
    mRevolveAction->setIcon(QIcon(":/Resources/revolve.png"));
    mRevolveAction->setStatusTip(tr("Test revol algorithm"));
    connect(mRevolveAction, SIGNAL(triggered()), this, SLOT(makeRevol()));

    mLoftAction = new QAction(tr("Loft"), this);
    mLoftAction->setIcon(QIcon(":/Resources/loft.png"));
    mLoftAction->setStatusTip(tr("Test loft algorithm"));
    connect(mLoftAction, SIGNAL(triggered()), this, SLOT(makeLoft()));

    mCutAction = new QAction(tr("Cut"), this);
    mCutAction->setIcon(QIcon(":/Resources/cut.png"));
    mCutAction->setStatusTip(tr("Boolean operation cut"));
    connect(mCutAction, SIGNAL(triggered()), this, SLOT(testCut()));

    mFuseAction = new QAction(tr("Fuse"), this);
    mFuseAction->setIcon(QIcon(":/Resources/fuse.png"));
    mFuseAction->setStatusTip(tr("Boolean operation fuse"));
    connect(mFuseAction, SIGNAL(triggered()), this, SLOT(testFuse()));

    mCommonAction = new QAction(tr("Common"), this);
    mCommonAction->setIcon(QIcon(":/Resources/common.png"));
    mCommonAction->setStatusTip(tr("Boolean operation common"));
    connect(mCommonAction, SIGNAL(triggered()), this, SLOT(testCommon()));

    mAboutAction = new QAction(tr("About"), this);
    mAboutAction->setStatusTip(tr("About the application"));
    mAboutAction->setIcon(QIcon(":/Resources/lamp.png"));
    connect(mAboutAction, SIGNAL(triggered()), this, SLOT(about()));

    mSelectEdges = new QAction(tr("Edge selection"), this);
    mSelectEdges->setStatusTip(tr("Change selections to edge selections"));
    connect(mSelectEdges, SIGNAL(triggered(bool)), this, SLOT(selectEdges()));
}

void MainWindow::createMenus()
{
    mFileMenu = menuBar()->addMenu(tr("&File"));
    mFileMenu->addAction(mExitAction);

    mViewMenu = menuBar()->addMenu(tr("&View"));
    mViewMenu->addAction(mViewZoomAction);
    mViewMenu->addAction(mViewPanAction);
    mViewMenu->addAction(mViewRotateAction);
    mViewMenu->addSeparator();
    mViewMenu->addAction(mViewResetAction);
    mViewMenu->addAction(mViewFitallAction);

    mPrimitiveMenu = menuBar()->addMenu(tr("&Primitive"));
    mPrimitiveMenu->addAction(mMakeBoxAction);
    mPrimitiveMenu->addAction(mMakeConeAction);
    mPrimitiveMenu->addAction(mMakeSphereAction);
    mPrimitiveMenu->addAction(mMakeCylinderAction);
    mPrimitiveMenu->addAction(mMakeTorusAction);

    mModelingMenu = menuBar()->addMenu(tr("&Modeling"));
    mModelingMenu->addAction(mFilletAction);
    mModelingMenu->addAction(mChamferAction);
    mModelingMenu->addAction(mExtrudeAction);
    mModelingMenu->addAction(mRevolveAction);
    mModelingMenu->addAction(mLoftAction);
    mModelingMenu->addSeparator();
    mModelingMenu->addAction(mCutAction);
    mModelingMenu->addAction(mFuseAction);
    mModelingMenu->addAction(mCommonAction);

    mHelpMenu = menuBar()->addMenu(tr("&Help"));
    mHelpMenu->addAction(mAboutAction);


    //Create extras menu
    mExtrasMenu = menuBar()->addMenu("&Extras");
    mExtrasMenu->addAction(mSelectEdges);

    QAction *action = new QAction(tr("Delete selection"), this);
    action->setStatusTip(tr("Delete item selected"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteSelections()));

    mExtrasMenu->addAction(action);

    action = new QAction(tr("Draw bounding box"), this);
    action->setStatusTip(tr("Draw bounding box in all itens in context iterating"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(drawBoundingBox()));

    mExtrasMenu->addAction(action);

    action = new QAction(tr("Unset color"), this);
    action->setStatusTip(tr("Unset color of all shapes in context"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(unsetColorOfAllShapes()));

    mExtrasMenu->addAction(action);

    action = new QAction(tr("Delete all"), this);
    action->setStatusTip(tr("Delete all shapes in context"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteAllShapes()));

    mExtrasMenu->addAction(action);

    //Create delete menu
    mDeleteMenu = menuBar()->addMenu("&Delete");

    action = new QAction(tr("Delete box"), this);
    action->setStatusTip(tr("Delete box using index of map"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteBox()));

    mDeleteMenu->addAction(action);

    action = new QAction(tr("Delete cone"), this);
    action->setStatusTip(tr("Delete cone using index of map"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteCone()));

    mDeleteMenu->addAction(action);

    action = new QAction(tr("Delete cone reducer"), this);
    action->setStatusTip(tr("Delete cone reducer using index of map"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteConeReducer()));

    mDeleteMenu->addAction(action);

    action = new QAction(tr("Delete sphere"), this);
    action->setStatusTip(tr("Delete sphere using index of map"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteSphere()));

    mDeleteMenu->addAction(action);

    action = new QAction(tr("Delete cylinder"), this);
    action->setStatusTip(tr("Delete cylinder using index of map"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteCylinder()));

    mDeleteMenu->addAction(action);

    action = new QAction(tr("Delete pie"), this);
    action->setStatusTip(tr("Delete pie using index of map"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deletePie()));

    mDeleteMenu->addAction(action);


    //Create modifry menu
    mModifyMenu = menuBar()->addMenu("&Modify");

    action = new QAction(tr("Modify box"), this);
    action->setStatusTip(tr("Delete box using index of map"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(modifyBox()));

    mModifyMenu->addAction(action);

}

void MainWindow::createToolBars()
{
    mNavigateToolBar = addToolBar(tr("&Navigate"));
    mNavigateToolBar->addAction(mViewZoomAction);
    mNavigateToolBar->addAction(mViewPanAction);
    mNavigateToolBar->addAction(mViewRotateAction);

    mViewToolBar = addToolBar(tr("&View"));
    mViewToolBar->addAction(mViewResetAction);
    mViewToolBar->addAction(mViewFitallAction);

    mPrimitiveToolBar = addToolBar(tr("&Primitive"));
    mPrimitiveToolBar->addAction(mMakeBoxAction);
    mPrimitiveToolBar->addAction(mMakeConeAction);
    mPrimitiveToolBar->addAction(mMakeSphereAction);
    mPrimitiveToolBar->addAction(mMakeCylinderAction);
    mPrimitiveToolBar->addAction(mMakeTorusAction);

    mModelingToolBar = addToolBar(tr("&Modeling"));
    mModelingToolBar->addAction(mFilletAction);
    mModelingToolBar->addAction(mChamferAction);
    mModelingToolBar->addAction(mExtrudeAction);
    mModelingToolBar->addAction(mRevolveAction);
    mModelingToolBar->addAction(mLoftAction);
    mModelingToolBar->addSeparator();
    mModelingToolBar->addAction(mCutAction);
    mModelingToolBar->addAction(mFuseAction);
    mModelingToolBar->addAction(mCommonAction);

    mHelpToolBar = addToolBar(tr("Help"));
    mHelpToolBar->addAction(mAboutAction);


}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About occQt"),
                       tr("<h2>occQt 3.0</h2>"
                          "<p>occQt is a demo applicaton about Qt and OpenCASCADE."));
}

void MainWindow::makeBox()
{
    TopoDS_Shape aTopoBox = BRepPrimAPI_MakeBox(3.0, 4.0, 5.0).Shape();
    Handle_AIS_Shape anAisBox = new AIS_Shape(aTopoBox);

    anAisBox->SetColor(Quantity_NOC_AZURE);

    mContext->Display(anAisBox);

    mapIntShapes.insert(0, anAisBox);
}

void MainWindow::makeCone()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 10.0, 0.0));

    TopoDS_Shape aTopoReducer = BRepPrimAPI_MakeCone(anAxis, 3.0, 1.5, 5.0).Shape();
    Handle_AIS_Shape anAisReducer = new AIS_Shape(aTopoReducer);

    anAisReducer->SetColor(Quantity_NOC_BISQUE);

    anAxis.SetLocation(gp_Pnt(8.0, 10.0, 0.0));
    TopoDS_Shape aTopoCone = BRepPrimAPI_MakeCone(anAxis, 3.0, 0.0, 5.0).Shape();
    Handle_AIS_Shape anAisCone = new AIS_Shape(aTopoCone);

    anAisCone->SetColor(Quantity_NOC_CHOCOLATE);

    mContext->Display(anAisReducer);
    mContext->Display(anAisCone);

    mapIntShapes.insert(1, anAisReducer);
    mapIntShapes.insert(2, anAisCone);
}

void MainWindow::makeSphere()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 20.0, 0.0));

    TopoDS_Shape aTopoSphere = BRepPrimAPI_MakeSphere(anAxis, 3.0).Shape();
    Handle_AIS_Shape anAisSphere = new AIS_Shape(aTopoSphere);

    anAisSphere->SetColor(Quantity_NOC_BLUE1);

    mContext->Display(anAisSphere);

    mapIntShapes.insert(3, anAisSphere);

}

void MainWindow::makeCylinder()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 30.0, 0.0));

    TopoDS_Shape aTopoCylinder = BRepPrimAPI_MakeCylinder(anAxis, 3.0, 5.0).Shape();
    Handle_AIS_Shape anAisCylinder = new AIS_Shape(aTopoCylinder);

    anAisCylinder->SetColor(Quantity_NOC_RED);

    anAxis.SetLocation(gp_Pnt(8.0, 30.0, 0.0));
    TopoDS_Shape aTopoPie = BRepPrimAPI_MakeCylinder(anAxis, 3.0, 5.0, M_PI_2 * 3.0).Shape();
    Handle_AIS_Shape anAisPie = new AIS_Shape(aTopoPie);

    anAisPie->SetColor(Quantity_NOC_TAN);

    mContext->Display(anAisCylinder);
    mContext->Display(anAisPie);

    mapIntShapes.insert(4, anAisCylinder);
    mapIntShapes.insert(5, anAisPie);
}

void MainWindow::makeTorus()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 40.0, 0.0));

    TopoDS_Shape aTopoTorus = BRepPrimAPI_MakeTorus(anAxis, 3.0, 1.0).Shape();
    Handle_AIS_Shape anAisTorus = new AIS_Shape(aTopoTorus);

    anAisTorus->SetColor(Quantity_NOC_YELLOW);

    anAxis.SetLocation(gp_Pnt(8.0, 40.0, 0.0));
    TopoDS_Shape aTopoElbow = BRepPrimAPI_MakeTorus(anAxis, 3.0, 1.0, M_PI_2).Shape();
    Handle_AIS_Shape anAisElbow = new AIS_Shape(aTopoElbow);

    anAisElbow->SetColor(Quantity_NOC_THISTLE);

    mContext->Display(anAisTorus);
    mContext->Display(anAisElbow);

    mapIntShapes.insert(6, anAisTorus);
    mapIntShapes.insert(7, anAisElbow);
}

void MainWindow::makeFillet()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 50.0, 0.0));

    TopoDS_Shape aTopoBox = BRepPrimAPI_MakeBox(anAxis, 3.0, 4.0, 5.0).Shape();
    BRepFilletAPI_MakeFillet MF(aTopoBox);

    // Add all the edges to fillet.
    for (TopExp_Explorer ex(aTopoBox, TopAbs_EDGE); ex.More(); ex.Next())
    {
        MF.Add(1.0, TopoDS::Edge(ex.Current()));
    }

    Handle_AIS_Shape anAisShape = new AIS_Shape(MF.Shape());
    anAisShape->SetColor(Quantity_NOC_VIOLET);

    mContext->Display(anAisShape);

    mapIntShapes.insert(8, anAisShape);

}

void MainWindow::makeChamfer()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(8.0, 50.0, 0.0));

    TopoDS_Shape aTopoBox = BRepPrimAPI_MakeBox(anAxis, 3.0, 4.0, 5.0).Shape();
    BRepFilletAPI_MakeChamfer MC(aTopoBox);
    TopTools_IndexedDataMapOfShapeListOfShape aEdgeFaceMap;

    TopExp::MapShapesAndAncestors(aTopoBox, TopAbs_EDGE, TopAbs_FACE, aEdgeFaceMap);

    for (Standard_Integer i = 1; i <= aEdgeFaceMap.Extent(); ++i)
    {
        TopoDS_Edge anEdge = TopoDS::Edge(aEdgeFaceMap.FindKey(i));
        TopoDS_Face aFace = TopoDS::Face(aEdgeFaceMap.FindFromIndex(i).First());

        MC.Add(0.6, 0.6, anEdge, aFace);
    }

    Handle_AIS_Shape anAisShape = new AIS_Shape(MC.Shape());
    anAisShape->SetColor(Quantity_NOC_TOMATO);

    mContext->Display(anAisShape);

    mapIntShapes.insert(9, anAisShape);
}

void MainWindow::makeExtrude()
{
    // prism a vertex result is an edge.
    TopoDS_Vertex aVertex = BRepBuilderAPI_MakeVertex(gp_Pnt(0.0, 60.0, 0.0));
    TopoDS_Shape aPrismVertex = BRepPrimAPI_MakePrism(aVertex, gp_Vec(0.0, 0.0, 5.0));
    Handle_AIS_Shape anAisPrismVertex = new AIS_Shape(aPrismVertex);

    // prism an edge result is a face.
    TopoDS_Edge anEdge = BRepBuilderAPI_MakeEdge(gp_Pnt(5.0, 60.0, 0.0), gp_Pnt(10.0, 60.0, 0.0));
    TopoDS_Shape aPrismEdge = BRepPrimAPI_MakePrism(anEdge, gp_Vec(0.0, 0.0, 5.0));
    Handle_AIS_Shape anAisPrismEdge = new AIS_Shape(aPrismEdge);

    // prism a wire result is a shell.
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(16.0, 60.0, 0.0));

    TopoDS_Edge aCircleEdge = BRepBuilderAPI_MakeEdge(gp_Circ(anAxis, 3.0));
    TopoDS_Wire aCircleWire = BRepBuilderAPI_MakeWire(aCircleEdge);
    TopoDS_Shape aPrismCircle = BRepPrimAPI_MakePrism(aCircleWire, gp_Vec(0.0, 0.0, 5.0));
    Handle_AIS_Shape anAisPrismCircle = new AIS_Shape(aPrismCircle);

    // prism a face or a shell result is a solid.
    anAxis.SetLocation(gp_Pnt(24.0, 60.0, 0.0));
    TopoDS_Edge aEllipseEdge = BRepBuilderAPI_MakeEdge(gp_Elips(anAxis, 3.0, 2.0));
    TopoDS_Wire aEllipseWire = BRepBuilderAPI_MakeWire(aEllipseEdge);
    TopoDS_Face aEllipseFace = BRepBuilderAPI_MakeFace(gp_Pln(gp::XOY()), aEllipseWire);
    TopoDS_Shape aPrismEllipse = BRepPrimAPI_MakePrism(aEllipseFace, gp_Vec(0.0, 0.0, 5.0));
    Handle_AIS_Shape anAisPrismEllipse = new AIS_Shape(aPrismEllipse);

    anAisPrismVertex->SetColor(Quantity_NOC_PAPAYAWHIP);
    anAisPrismEdge->SetColor(Quantity_NOC_PEACHPUFF);
    anAisPrismCircle->SetColor(Quantity_NOC_PERU);
    anAisPrismEllipse->SetColor(Quantity_NOC_PINK);

    mContext->Display(anAisPrismVertex);
    mContext->Display(anAisPrismEdge);
    mContext->Display(anAisPrismCircle);
    mContext->Display(anAisPrismEllipse);

    mapIntShapes.insert(10, anAisPrismVertex);
    mapIntShapes.insert(11, anAisPrismEdge);
    mapIntShapes.insert(12, anAisPrismCircle);
    mapIntShapes.insert(13, anAisPrismEllipse);
}

void MainWindow::makeRevol()
{
    gp_Ax1 anAxis;

    // revol a vertex result is an edge.
    anAxis.SetLocation(gp_Pnt(0.0, 70.0, 0.0));
    TopoDS_Vertex aVertex = BRepBuilderAPI_MakeVertex(gp_Pnt(2.0, 70.0, 0.0));
    TopoDS_Shape aRevolVertex = BRepPrimAPI_MakeRevol(aVertex, anAxis);
    Handle_AIS_Shape anAisRevolVertex = new AIS_Shape(aRevolVertex);

    // revol an edge result is a face.
    anAxis.SetLocation(gp_Pnt(8.0, 70.0, 0.0));
    TopoDS_Edge anEdge = BRepBuilderAPI_MakeEdge(gp_Pnt(6.0, 70.0, 0.0), gp_Pnt(6.0, 70.0, 5.0));
    TopoDS_Shape aRevolEdge = BRepPrimAPI_MakeRevol(anEdge, anAxis);
    Handle_AIS_Shape anAisRevolEdge = new AIS_Shape(aRevolEdge);

    // revol a wire result is a shell.
    anAxis.SetLocation(gp_Pnt(20.0, 70.0, 0.0));
    anAxis.SetDirection(gp::DY());

    TopoDS_Edge aCircleEdge = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(15.0, 70.0, 0.0), gp::DZ()), 1.5));
    TopoDS_Wire aCircleWire = BRepBuilderAPI_MakeWire(aCircleEdge);
    TopoDS_Shape aRevolCircle = BRepPrimAPI_MakeRevol(aCircleWire, anAxis, M_PI_2);
    Handle_AIS_Shape anAisRevolCircle = new AIS_Shape(aRevolCircle);

    // revol a face result is a solid.
    anAxis.SetLocation(gp_Pnt(30.0, 70.0, 0.0));
    anAxis.SetDirection(gp::DY());

    TopoDS_Edge aEllipseEdge = BRepBuilderAPI_MakeEdge(gp_Elips(gp_Ax2(gp_Pnt(25.0, 70.0, 0.0), gp::DZ()), 3.0, 2.0));
    TopoDS_Wire aEllipseWire = BRepBuilderAPI_MakeWire(aEllipseEdge);
    TopoDS_Face aEllipseFace = BRepBuilderAPI_MakeFace(gp_Pln(gp::XOY()), aEllipseWire);
    TopoDS_Shape aRevolEllipse = BRepPrimAPI_MakeRevol(aEllipseFace, anAxis, M_PI_4);
    Handle_AIS_Shape anAisRevolEllipse = new AIS_Shape(aRevolEllipse);

    anAisRevolVertex->SetColor(Quantity_NOC_LIMEGREEN);
    anAisRevolEdge->SetColor(Quantity_NOC_LINEN);
    anAisRevolCircle->SetColor(Quantity_NOC_MAGENTA1);
    anAisRevolEllipse->SetColor(Quantity_NOC_MAROON);

    mContext->Display(anAisRevolVertex);
    mContext->Display(anAisRevolEdge);
    mContext->Display(anAisRevolCircle);
    mContext->Display(anAisRevolEllipse);

    mapIntShapes.insert(14, anAisRevolVertex);
    mapIntShapes.insert(15, anAisRevolEdge);
    mapIntShapes.insert(16, anAisRevolCircle);
    mapIntShapes.insert(17, anAisRevolEllipse);
}

void MainWindow::makeLoft()
{
    // bottom wire.
    TopoDS_Edge aCircleEdge = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(0.0, 80.0, 0.0), gp::DZ()), 1.5));
    TopoDS_Wire aCircleWire = BRepBuilderAPI_MakeWire(aCircleEdge);

    // top wire.
    BRepBuilderAPI_MakePolygon aPolygon;
    aPolygon.Add(gp_Pnt(-3.0, 77.0, 6.0));
    aPolygon.Add(gp_Pnt(3.0, 77.0, 6.0));
    aPolygon.Add(gp_Pnt(3.0, 83.0, 6.0));
    aPolygon.Add(gp_Pnt(-3.0, 83.0, 6.0));
    aPolygon.Close();

    BRepOffsetAPI_ThruSections aShellGenerator;
    BRepOffsetAPI_ThruSections aSolidGenerator(true);

    aShellGenerator.AddWire(aCircleWire);
    aShellGenerator.AddWire(aPolygon.Wire());

    aSolidGenerator.AddWire(aCircleWire);
    aSolidGenerator.AddWire(aPolygon.Wire());

    // translate the solid.
    gp_Trsf aTrsf;
    aTrsf.SetTranslation(gp_Vec(18.0, 0.0, 0.0));
    BRepBuilderAPI_Transform aTransform(aSolidGenerator.Shape(), aTrsf);

    Handle_AIS_Shape anAisShell = new AIS_Shape(aShellGenerator.Shape());
    Handle_AIS_Shape anAisSolid = new AIS_Shape(aTransform.Shape());

    anAisShell->SetColor(Quantity_NOC_OLIVEDRAB);
    anAisSolid->SetColor(Quantity_NOC_PEACHPUFF);

    mContext->Display(anAisShell);
    mContext->Display(anAisSolid);

    mapIntShapes.insert(18, anAisShell);
    mapIntShapes.insert(19, anAisSolid);
}

void MainWindow::testCut()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 90.0, 0.0));

    TopoDS_Shape aTopoBox = BRepPrimAPI_MakeBox(anAxis, 3.0, 4.0, 5.0).Shape();
    TopoDS_Shape aTopoSphere = BRepPrimAPI_MakeSphere(anAxis, 2.5).Shape();
    TopoDS_Shape aCuttedShape1 = BRepAlgoAPI_Cut(aTopoBox, aTopoSphere);
    TopoDS_Shape aCuttedShape2 = BRepAlgoAPI_Cut(aTopoSphere, aTopoBox);

    gp_Trsf aTrsf;
    aTrsf.SetTranslation(gp_Vec(8.0, 0.0, 0.0));
    BRepBuilderAPI_Transform aTransform1(aCuttedShape1, aTrsf);

    aTrsf.SetTranslation(gp_Vec(16.0, 0.0, 0.0));
    BRepBuilderAPI_Transform aTransform2(aCuttedShape2, aTrsf);

    Handle_AIS_Shape anAisBox = new AIS_Shape(aTopoBox);
    Handle_AIS_Shape anAisSphere = new AIS_Shape(aTopoSphere);
    Handle_AIS_Shape anAisCuttedShape1 = new AIS_Shape(aTransform1.Shape());
    Handle_AIS_Shape anAisCuttedShape2 = new AIS_Shape(aTransform2.Shape());

    anAisBox->SetColor(Quantity_NOC_SPRINGGREEN);
    anAisSphere->SetColor(Quantity_NOC_STEELBLUE);
    anAisCuttedShape1->SetColor(Quantity_NOC_TAN);
    anAisCuttedShape2->SetColor(Quantity_NOC_SALMON);

    mContext->Display(anAisBox);
    mContext->Display(anAisSphere);
    mContext->Display(anAisCuttedShape1);
    mContext->Display(anAisCuttedShape2);

    mapIntShapes.insert(20, anAisBox);
    mapIntShapes.insert(21, anAisSphere);
    mapIntShapes.insert(22, anAisCuttedShape1);
    mapIntShapes.insert(23, anAisCuttedShape2);
}

void MainWindow::testFuse()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 100.0, 0.0));

    TopoDS_Shape aTopoBox = BRepPrimAPI_MakeBox(anAxis, 3.0, 4.0, 5.0).Shape();
    TopoDS_Shape aTopoSphere = BRepPrimAPI_MakeSphere(anAxis, 2.5).Shape();
    TopoDS_Shape aFusedShape = BRepAlgoAPI_Fuse(aTopoBox, aTopoSphere);

    gp_Trsf aTrsf;
    aTrsf.SetTranslation(gp_Vec(8.0, 0.0, 0.0));
    BRepBuilderAPI_Transform aTransform(aFusedShape, aTrsf);

    Handle_AIS_Shape anAisBox = new AIS_Shape(aTopoBox);
    Handle_AIS_Shape anAisSphere = new AIS_Shape(aTopoSphere);
    Handle_AIS_Shape anAisFusedShape = new AIS_Shape(aTransform.Shape());

    anAisBox->SetColor(Quantity_NOC_SPRINGGREEN);
    anAisSphere->SetColor(Quantity_NOC_STEELBLUE);
    anAisFusedShape->SetColor(Quantity_NOC_ROSYBROWN);

    mContext->Display(anAisBox);
    mContext->Display(anAisSphere);
    mContext->Display(anAisFusedShape);

    mapIntShapes.insert(24, anAisBox);
    mapIntShapes.insert(25, anAisSphere);
    mapIntShapes.insert(26, anAisFusedShape);

}

void MainWindow::testCommon()
{
    gp_Ax2 anAxis;
    anAxis.SetLocation(gp_Pnt(0.0, 110.0, 0.0));

    TopoDS_Shape aTopoBox = BRepPrimAPI_MakeBox(anAxis, 3.0, 4.0, 5.0).Shape();
    TopoDS_Shape aTopoSphere = BRepPrimAPI_MakeSphere(anAxis, 2.5).Shape();
    TopoDS_Shape aCommonShape = BRepAlgoAPI_Common(aTopoBox, aTopoSphere);

    gp_Trsf aTrsf;
    aTrsf.SetTranslation(gp_Vec(8.0, 0.0, 0.0));
    BRepBuilderAPI_Transform aTransform(aCommonShape, aTrsf);

    Handle_AIS_Shape anAisBox = new AIS_Shape(aTopoBox);
    Handle_AIS_Shape anAisSphere = new AIS_Shape(aTopoSphere);
    Handle_AIS_Shape anAisCommonShape = new AIS_Shape(aTransform.Shape());

    anAisBox->SetColor(Quantity_NOC_SPRINGGREEN);
    anAisSphere->SetColor(Quantity_NOC_STEELBLUE);
    anAisCommonShape->SetColor(Quantity_NOC_ROYALBLUE);

    mContext->Display(anAisBox);
    mContext->Display(anAisSphere);
    mContext->Display(anAisCommonShape);

    mapIntShapes.insert(27, anAisBox);
    mapIntShapes.insert(28, anAisSphere);
    mapIntShapes.insert(29, anAisCommonShape);
}

void MainWindow::timerRedraw()
{
    this->resize(this->width() * 2, this->height() * 2);
}

void MainWindow::selectEdges()
{
    //    mContext->Select(this->x() - this->width(), this->y() - this->height(), this->width(), this->height(), occView->getMyView(), true);

    mContext->CloseAllContexts();
    mContext->OpenLocalContext();
    mContext->ActivateStandardMode(TopAbs_FACE);

    //Roman Lygin - start of the test
    //select all edges of all displayed objects
    AIS_ListOfInteractive aDisplayedList;
    Standard_Boolean aNeutralPointOnly = Standard_True;
    mContext->DisplayedObjects (aDisplayedList, aNeutralPointOnly);

    AIS_ListIteratorOfListOfInteractive anIter (aDisplayedList);
    for (; anIter.More(); anIter.Next())
    {
        Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast (anIter.Value());

        if (!aShape.IsNull()) {
            TopExp_Explorer anExp (aShape->Shape(), TopAbs_EDGE);
            for (; anExp.More(); anExp.Next()) {
                mContext->AddOrRemoveSelected (anExp.Current(), Standard_False);
            }
        }
    }
    mContext->UpdateCurrentViewer(); //now update the context

}

void MainWindow::deleteSelections()
{
    for(mContext->InitCurrent(); mContext->MoreCurrent(); mContext->NextCurrent())
    {
        mContext->Erase(mContext->Current(), true);
    }

    mContext->ClearSelected();
}

void MainWindow::drawBoundingBox()
{
    mContext->CloseAllContexts();
    mContext->OpenLocalContext();

    //Roman Lygin - start of the test
    //select all edges of all displayed objects
    AIS_ListOfInteractive aDisplayedList;
    Standard_Boolean aNeutralPointOnly = Standard_True;
    mContext->DisplayedObjects (aDisplayedList, aNeutralPointOnly);

    AIS_ListIteratorOfListOfInteractive anIter (aDisplayedList);
    for (; anIter.More(); anIter.Next())
    {
//        http://www.opencascade.com/content/how-get-proper-bounding-box-any-shape
        Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast (anIter.Value());

        Standard_Real aDeflection = 0.001, deflection;
        Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;

        Bnd_Box box;
        BRepBndLib::Add(aShape->Shape(), box);
        box.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
        deflection= MAX3( aXmax-aXmin , aYmax-aYmin , aZmax-aZmin)*aDeflection;

        BRepMesh_IncrementalMesh Inc(aShape->Shape(), deflection);

        box.SetVoid();
        BRepBndLib::Add(aShape->Shape(), box);
        box.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);


        TopoDS_Shape aTopoBox = BRepPrimAPI_MakeBox(box.CornerMin(), box.CornerMax()).Shape();
        Handle_AIS_Shape anAisBox = new AIS_Shape(aTopoBox);

        anAisBox->SetColor(Quantity_NOC_AZURE);
        anAisBox->SetTransparency(0.8);

        mContext->Display(anAisBox,false);

    }

    mContext->UpdateCurrentViewer(); //now update the context
}

void MainWindow::unsetColorOfAllShapes()
{

    //    mContext->CloseAllContexts();
    //    mContext->OpenLocalContext();
    //    mContext->ActivateStandardMode(TopAbs_EDGE);

    //Roman Lygin - start of the test
    //select all edges of all displayed objects
    AIS_ListOfInteractive aDisplayedList;
    Standard_Boolean aNeutralPointOnly = Standard_True;
    mContext->DisplayedObjects (aDisplayedList, aNeutralPointOnly);

    AIS_ListIteratorOfListOfInteractive anIter (aDisplayedList);
    for (; anIter.More(); anIter.Next())
    {
        Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast (anIter.Value());


        if (!aShape.IsNull()) {

            aShape->UnsetColor();

        }
    }

    mContext->UpdateCurrentViewer(); //now update the context

}

void MainWindow::deleteAllShapes()
{
#if 1
    //    mContext->CloseAllContexts();
    //    mContext->OpenLocalContext();
    //    mContext->ActivateStandardMode(TopAbs_EDGE);

    //Roman Lygin - start of the test
    //select all edges of all displayed objects
    AIS_ListOfInteractive aDisplayedList;
    Standard_Boolean aNeutralPointOnly = Standard_True;
    mContext->DisplayedObjects (aDisplayedList, aNeutralPointOnly);

    AIS_ListIteratorOfListOfInteractive anIter (aDisplayedList);
    for (; anIter.More(); anIter.Next())
    {
        Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast (anIter.Value());


        if (!aShape.IsNull()) {

            aShape->UnsetColor();

            mContext->Erase(aShape, false);

        }
    }

    mContext->UpdateCurrentViewer(); //now update the context
#endif

}

void MainWindow::deleteBox()
{
    if(!mapIntShapes[0].IsNull())
    {
        mContext->Erase(mapIntShapes[0]);
    }
}

//http://www.opencascade.com/content/modify-shape
//http://www.opencascade.com/content/how-can-i-use-breptoolsreshape
void MainWindow::modifyBox()
{
    if(!mapIntShapes[0].IsNull())
    {
//        mContext->Erase(mapIntShapes[0]);
//        Topo shape = mapIntShapes[0]->Shape();

        TopoDS_Shape aTopoBox = BRepPrimAPI_MakeBox(5.0, 2.0, 2.0).Shape();
//        Handle_AIS_Shape anAisBox = new AIS_Shape(aTopoBox);
        BRepTools_ReShape reshape;

        reshape.Replace(mapIntShapes[0]->Shape(), aTopoBox, true);
        TopoDS_Shape result = reshape.Apply(mapIntShapes[0]->Shape());

        Handle(AIS_Shape) aisShape = new AIS_Shape(result);

        mContext->Erase(mapIntShapes[0], true);
        mapIntShapes.remove(0);

        mapIntShapes.insert(0, aisShape);
        mContext->Display(mapIntShapes[0], true);
    }
}

void MainWindow::deleteCone()
{
    if(!mapIntShapes[2].IsNull())
    {
        mContext->Erase(mapIntShapes[2]);
    }
}

void MainWindow::deleteConeReducer()
{
    if(!mapIntShapes[1].IsNull())
    {
        mContext->Erase(mapIntShapes[1]);
    }
}

void MainWindow::deleteSphere()
{
    if(!mapIntShapes[3].IsNull())
    {
        mContext->Erase(mapIntShapes[3]);
    }
}

void MainWindow::deleteCylinder()
{
    if(!mapIntShapes[4].IsNull())
    {
        mContext->Erase(mapIntShapes[4]);
    }
}

void MainWindow::deletePie()
{
    if(!mapIntShapes[5].IsNull())
    {
        mContext->Erase(mapIntShapes[5]);
    }
}

