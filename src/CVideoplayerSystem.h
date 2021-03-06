/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#pragma once

#include <IPluginD3D.h>
#include "IPluginVideoplayer.h"

#include <Game.h>
#include <map>
#include <Playlist/CAutoPlaylists.h>

namespace VideoplayerPlugin
{
    /**
    * @brief Structure to hold material override information
    * Is used internally to restore materials on video ends/resets...
    */
    typedef struct SMaterialOverride_
    {
        IVideoplayer* pVideo; //!< Video assigned by this override
        IMaterial* pMaterial; //!< Material this override modifies
        int nTextureslot; //!< Textureslot to be modified
        bool bRecommendedSettings; //!< automatically sets some sensible shader parameters

        SMaterialOverride_()
        {
            Reset();
        };

        /**
        * @brief Reset the structure for reuse
        */
        void Reset()
        {
            pVideo = NULL;
            pMaterial = NULL;
            nTextureslot = 0;
            bRecommendedSettings = false;
        };

        /**
        * @brief Fill the structure with new parameters
        * @param _pVideo pointer to the source video interface
        * @param _pMaterial pointer to the material interface affected
        * @param _nTextureslot texture slot to be overridden
        * @param _bRecommendedSettings automatically set some shader parameters
        */
        void Set( IVideoplayer* _pVideo, IMaterial* _pMaterial, int _nTextureslot = EFTT_DIFFUSE, bool _bRecommendedSettings = true )
        {
            pVideo = _pVideo;
            pMaterial = _pMaterial;
            nTextureslot = _nTextureslot;
            bRecommendedSettings = _bRecommendedSettings;
        };
    } SMaterialOverride;

    typedef std::list<S2DVideo> t2DVideos; //!< hold 2D video information
    typedef std::vector<SMaterialOverride> tOverrideSet; //!< hold material override infomation
    typedef std::map<IMaterial*, tOverrideSet> tOverrideMap; //!< material 1:N override relation
    typedef std::map<IMaterial*, IMaterial*> tOrginalMaterialMap; //!< modified material N;1 orginal material relation
    typedef std::map<int, IVideoplayer*> tVideoIDMap; //!< videoid 1;1 video interface relation
    typedef std::map<IVideoplayerPlaylist*, IVideoplayerPlaylist*> tVideoPlaylistMap; //!< simply for simpler loopkup

    /**
    * @brief Type to identify what is currently displayed on the screen.
    * This type is based on the old CryEngine Bitmap classes.
    */
    typedef enum
    {
        eSS_EditorScreen, //!< In editor
        eSS_Initialize, //!< Startup
        eSS_StartScreen, //!< Splashscreen
        eSS_Menu, //!< Menu (No level loaded)
        eSS_LoadingScreen, //!< Loading level
        eSS_BlockedScreen, //!< Ingame, Game Blocked/Paused during Video Playback
        eSS_PausedScreen, //!< Ingame Paused (Menu)
        eSS_InGameScreen, //!< Playing Game
    } EScreenState;

    /**
    * @brief Videoplayer System manages the resources
    * Uses CryExtension so it can be instantiated in CryGame.
    * Also this class redirects D3D events to the resource specific classes.
    */
    class CVideoplayerSystem :
        public IPluginVideoplayer,
        public D3DPlugin::ID3DEventListener,
        public ILevelSystemListener,
        public IGameFrameworkListener,
        public ISystemEventListener,
        public IActionListener
    {
        public:
            /**
            * @brief Initialize the global videoplayer plugin singleton.
            * @return Pointer to system implementation class.
            */
            CVideoplayerSystem();
            ~CVideoplayerSystem();

            PluginManager::IPluginBase* GetBase();

        public:
            bool m_bEditing; //!< Editor is in editing mode

            /* CVars */
            int vp_playbackmode; //!< @see ePlaybackMode

            float vp_seekthreshold; //!< Threshold in seconds to trigger seeks @see eDropMode
            float vp_dropthreshold; //!< Threshold in seconds to trigger drops @see eDropMode
            float vp_dropmaxduration; //!< Maximal duration to drop at one time before outputting a frame again

            ColorB vp_defaultcolor; //!< Default Video Color (if it hasn't started playing)

            CAutoPlaylists* m_pAutoPlaylists; //!< Automatic Playlist

        private:

            int m_nFreeVideoId; //!< next video ID / free ID (could overflow in an extremly unlikly use case of creating 2^31-1 videos)
            tVideoIDMap m_pVideos; //!< videoid 1;1 video interface relation
            tVideoPlaylistMap m_pPlaylists; //!< simply for simpler loopkup and cleanup
            tOrginalMaterialMap m_pMaterials; //!< modified material N;1 orginal material relation
            tOverrideMap m_Overrides; //!< material 1:N override relation
            t2DVideos m_p2DVideos; //!< 2D video information of active 2D video window

            int m_nGameLoopActive; //!< If <0 then game loop inactive
            int m_nD3DActive; //!< If <0 then the D3D system is inactive
            float m_fFrameTime; //!< current frame time

            IActionFilter* m_pOnlySkipFilter; //!< Input filter for Skip Events

            EScreenState m_currentScreenState; //!< current screen state
            bool m_bBlocked; //!< game is currently blocked/paused for videos

            /**
            * @brief advance/render all resources
            * @param fDeltaTime time passed since last call in seconds
            */
            virtual void AdvanceAll( float fDeltaTime );

            /**
            * @brief draw all 2D resources
            */
            virtual void DrawAll();
        public:

            // see ID3DListener
            virtual void OnPrePresent() {};
            virtual void OnPostPresent() {};
            virtual void OnPreReset();
            virtual void OnPostReset();
            virtual void OnPostBeginScene();

            // see IGameFrameworkListener
            virtual void OnPostUpdate( float fDeltaTime );
            virtual void OnPreRender();
            virtual void OnSaveGame( ISaveGame* pSaveGame ) {};
            virtual void OnLoadGame( ILoadGame* pLoadGame );
            virtual void OnLevelEnd( const char* nextLevel );
            virtual void OnActionEvent( const SActionEvent& event );

            // see ISystemEventListener
            virtual void OnSystemEvent( ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam );

            // see ILevelSystemListener
            virtual void OnLevelNotFound( const char* levelName ) {};
            virtual void OnLoadingStart( ILevelInfo* pLevel );
            virtual void OnLoadingComplete( ILevel* pLevel );
            virtual void OnLoadingError( ILevelInfo* pLevel, const char* error ) {};
            virtual void OnLoadingProgress( ILevelInfo* pLevel, int progressAmount );
            virtual void OnUnloadComplete( ILevel* pLevel ) {};

            // see IActionListener
            virtual void OnAction( const ActionId& action, int activationMode, float value );

            /**
            * @brief Set the screen state
            * @param eState New screen state
            * @param bJustSet Don't trigger menu changes (used internally)
            * @attention in editor the screen state is always set to editor
            */
            virtual void SetScreenState( const EScreenState eState, bool bJustSet = false );

            /**
            * @brief Retrieve the screen state
            * @attention in editor the screen state is always set to editor
            */
            virtual EScreenState GetScreenState() const;

            /**
            * @brief Show/Hide the Menu
            * @param bShow Show/Hide
            * @attention The UI Events OnStopIngameMenu, OnStartIngameMenu and OnSystemStarted must be implemented like in the default SDK.
            */
            virtual void ShowMenu( bool bShow );

            /**
            * @brief Is the Menu visible
            * @return menu visible
            */
            virtual bool IsMenuVisible();

            /**
            * @brief Freeze the game but show the pause menu
            * @param bBlock Resume / Pause
            * @attention Flownode events are not transmitted while the game is paused/blocked,
            */
            virtual void BlockGameForVideo( bool bBlock );

            /**
            * @brief Retrieve Game state
            * @return Game currently blocked?
            * @see BlockGameForVideo
            */
            virtual bool IsGameBlocked();

            /**
            * @brief Game loop active or not? (e.g. in editing mode of the editor the game loop is not active)
            * @return game loop active?
            */
            bool IsGameLoopActive();

            /**
            * @brief D3D System State
            * @return D3D hooks are working or not
            */
            bool IsD3DActive();

            // see IPluginVideoplayer
            bool Initialize( );
            IVideoplayer* CreateVideoplayer( const char* sType = "WebM" );
            void DeleteVideoplayer( IVideoplayer* pVideoplayer );
            IVideoplayer* GetVideoplayerById( int nVideoID = -1 );

            IVideoplayerPlaylist* CreatePlaylist( bool bShowMenuOnEndDefault = false );
            void DeletePlaylist( IVideoplayerPlaylist* pPlaylist );

            S2DVideo* Create2DVideo();
            void Delete2DVideo( S2DVideo* p2DVideo );

            void RememberMaterial( IMaterial* mat );
            bool RestoreMaterial( IMaterial* mat, bool bResetOverride = false );

            IMaterial* CreateMaterial( IVideoplayer* pVideo, const char* sMaterial, int nMtlFlags = 0 );
            bool OverrideMaterial( IVideoplayer* pVideo, IMaterial* pMaterial, int nSubmat = 0, int nTextureslot = EFTT_DIFFUSE, bool bRecommendedSettings = true );
            bool ResetMaterial( IMaterial* pMaterial, int nSubmat = 0, bool bResetOverride = false );

            void RestoreMaterials( IVideoplayer* pVideo, bool bResetOverride = false );
            void OverrideMaterials( IVideoplayer* pVideo );

            void ReleaseMaterials();
    };
}

extern VideoplayerPlugin::CVideoplayerSystem* gVideoplayerSystem; //!< Global internal Videoplayer System Pointer