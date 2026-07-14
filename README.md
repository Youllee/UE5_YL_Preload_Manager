# Youllee's Preload Manager

Unreal Engine에서 자주 사용하는 데이터 에셋을 Editor World와 Game World에 미리 로드해두기 위한 플러그인입니다.

프로젝트 설정에서 `DataAsset`, `DataTable`, `DataRegistry`를 등록하면, 지정한 월드 수명주기에 맞춰 에셋을 동기 로드하고 강한 참조로 유지합니다.

게임 시스템이 데이터를 요청하기 전에 필요한 에셋을 미리 준비해두고 싶을 때 사용할 수 있습니다.

![YL Preload Manager Settings](Docs/YL_Preload_Manager_Setting.png)

## 주요 기능

- 'DataAsset', 'DataTable', 'DataRegistry' Preload
- Preload 시점 설정 (Editor World / Game World)

## 설치

`YLPreloadManager` 폴더를 프로젝트의 `Plugins` 폴더 아래에 복사합니다.

```text
YourProject/
  Plugins/
    YLPreloadManager/
      YLPreloadManager.uplugin
      Source/
      Config/
      Resources/
```

그 후 Unreal Editor를 다시 실행하고, 필요하면 플러그인을 활성화한 뒤 프로젝트를 다시 빌드합니다.

## 설정 위치

프로젝트 설정에서 아래 경로로 이동합니다.

```text
Project Settings > Game > Youllee's Preload Manager
```

설정 화면에는 세 종류의 프리로드 목록이 있습니다.

- **Data Assets**: `UDataAsset` 기반 에셋을 등록합니다.
- **Data Tables**: `UDataTable` 에셋을 등록합니다.
- **Data Registries**: `UDataRegistry` 에셋을 등록합니다.

각 항목에는 다음 옵션이 있습니다.

- **Preload on Editor**: 에디터 월드에서 프리로드합니다.
- **Preload on Game**: 게임 월드에서 프리로드합니다.

## 에셋 수명

이 플러그인은 로드한 에셋을 `TStrongObjectPtr`로 보관합니다.
따라서 프리로드 목록에 들어간 에셋은 해당 수명주기가 끝날 때까지 GC 대상이 되지 않습니다.

- Preload on Editor : true인 경우, Engine 초기화 이후 로드되고, Module 셧다운 시 언로드 됩니다.
- Preload on Game : true인 경우, GameWorld 초기화 이후 로드되고, GameWorld 클린업 시 언로드 됩니다.

## 갱신 시점

1. Editor에서 Preload Manager Setting이 변경된 경우
2. Editor에서 Preload된 Assest이 저장된 경우

기존에 로드된 에셋을 리셋하고, 다시 로드합니다.

## 주의사항

- 프리로드는 동기 로드로 처리됩니다. 에셋의 용량이 크거나 지나치게 많은 에셋을 등록하면, 프리로드 시 비용이 커질 수 있습니다.
- 이 플러그인은 에셋을 미리 로드하고 참조를 유지하는 용도이며, 모든 데이터 초기화 순서를 자동으로 보장하지 않습니다.

## 라이선스

MIT License. 자세한 내용은 [LICENSE](LICENSE)를 참고하세요.
