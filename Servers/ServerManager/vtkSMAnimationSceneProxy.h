/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAnimationSceneProxy - proxy for vtkAnimationScene
// .SECTION Description
// Proxy for animation scene. A scene is an animation setup that can be played.
// Also supports writing out animation images (movie) and animation geometry.
// Like all animation proxies, this is a client side proxy with not server 
// side VTK objects created.
// .SECTION See Also
// vtkAnimationScene vtkSMAnimationCueProxy

#ifndef __vtkSMAnimationSceneProxy_h
#define __vtkSMAnimationSceneProxy_h

#include "vtkSMAnimationCueProxy.h"

class vtkAnimationScene;
class vtkCollection;
class vtkCollectionIterator;
class vtkGenericMovieWriter;
class vtkImageWriter;
class vtkSMAnimationSceneProxyInternals;
class vtkSMRenderModuleProxy;
class vtkSMAbstractViewModuleProxy;

class VTK_EXPORT vtkSMAnimationSceneProxy : public vtkSMAnimationCueProxy
{
public:
  static vtkSMAnimationSceneProxy* New();
  vtkTypeRevisionMacro(vtkSMAnimationSceneProxy, vtkSMAnimationCueProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Start playing the animation. On every \c Tick, 
  // \c vtkCommand::AnimationCueTickEvent is fired. One can call \c Stop()
  // in the event handler for this event to abort playing of the animation.
  // If \c Loop is set, the animation will be played in a loop.
  // This function returns only after the animation playing has stopped.
  void Play();

  // Description:
  // Stops playing the animation. This method has any effect only when
  // called within the vtkCommand::AnimationCueTickEvent event handler. 
  // This event is fired when playing the animation.
  void Stop();

  // Description:
  // Returns the status of the player. True when the animation is being played.
  int IsInPlay();
 
  // Description:
  // Set/Get if the animation should be played in a loop.
  void SetLoop(int loop);
  int GetLoop();

  // Description;
  // Set/Get the frame rate for the animation. Frame rate is used only when
  // the play mode is \c vtkAnimationScene::PLAYMODE_SEQUENCE.
  void SetFrameRate(double framerate);
  double GetFrameRate();

  //BTX
  enum 
    {
    SEQUENCE=0,
    REALTIME=1
    };
  //ETX
  // Description:
  // Note that when the play mode is set to Real Time, cacheing is
  // disabled.
  virtual void SetPlayMode(int mode);
  int GetPlayMode();

  void AddCue(vtkSMProxy* cue);
  void RemoveCue(vtkSMProxy* cue);
 
  // Description:
  // Set if caching is enabled.
  // This method synchronizes the cahcing flag on every cue.
  virtual void SetCaching(int enable); 

  // Description:
  // This method calls InvalidateAllGeometries on the vtkSMRenderModuleProxy.
  // However, to minimize the calls to InvalidateAllGeometries, this call
  // keeps a flag indicating if CacheUpdate was ever called on the 
  // Render Module and calls InvalidateAllGeometries only of the flag
  // is set.
  void CleanCache();
  
  // Description:
  // Add view module that is involved in the animation generated by this scene.
  // When playing animation, the scene proxy will call Render()
  // and CacheUpdate() on view modules that it is aware of. Also, while saving,
  // geometry or images, the scene considers only the view modules it is aware of.
  void AddViewModule(vtkSMAbstractViewModuleProxy*);
  void RemoveViewModule(vtkSMAbstractViewModuleProxy*);
  void RemoveAllViewModules();

  // Description:
  // API to get the view modules.
  unsigned int GetNumberOfViewModules();
  vtkSMAbstractViewModuleProxy* GetViewModule(unsigned int cc);
 
  // Description:
  // Method to set the current time. This updates the proxies to reflect the state
  // at the indicated time.
  void SetAnimationTime(double time);

protected:
  vtkSMAnimationSceneProxy();
  ~vtkSMAnimationSceneProxy();

  virtual void CreateVTKObjects();
  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);
  virtual void InitializeObservers(vtkAnimationCue* cue); 

  // Set when CacheUpdate() is called on any view.
  int GeometryCached; 

  // Description:
  // Callbacks for corresponding Cue events. The argument must be 
  // casted to vtkAnimationCue::AnimationCueInfo.
  virtual void StartCueInternal(void* info);
  virtual void TickInternal(void* info);
  virtual void EndCueInternal(void* info);
  void CacheUpdate(void* info);
  
  vtkCollection* AnimationCueProxies;
  vtkCollectionIterator* AnimationCueProxiesIterator;

  int PlayMode;

  //BTX
  friend class vtkSMAnimationSceneImageWriter;
  int OverrideStillRender;
  vtkSetMacro(OverrideStillRender, int);
  //ETX
private:
  vtkSMAnimationSceneProxy(const vtkSMAnimationSceneProxy&); // Not implemented.
  void operator=(const vtkSMAnimationSceneProxy&); // Not implemented.

  vtkSMAnimationSceneProxyInternals* Internals;
};


#endif

