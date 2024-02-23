import { asset } from '@kit.AssetStoreKit';
import { stringToArray, addAssetPromise, removeAssetPromise, queryContentPromise, queryAttrPromise,
  addAssetPromiseResult, arrayToString } from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

export default function addAssetPromiseTest() {
  describe('addFuncPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('addSingleData', 0, async () => {
      let alias = 'addSingleData'
      expect(await addAssetPromise(alias)).assertTrue()

      let res = await queryContentPromise(alias)
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray("demo_pwd"))
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('addAllTags', 0, async () => {
      let alias = 'addAllTags'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label1"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_2, stringToArray("normal_label2"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_3, stringToArray("normal_label3"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_4, stringToArray("normal_label4"))
      attr.set(asset.Tag.DATA_LABEL_CRITICAL_1, stringToArray("critical_label1"))
      attr.set(asset.Tag.DATA_LABEL_CRITICAL_2, stringToArray("critical_label2"))
      attr.set(asset.Tag.DATA_LABEL_CRITICAL_3, stringToArray("critical_label3"))
      attr.set(asset.Tag.DATA_LABEL_CRITICAL_4, stringToArray("critical_label4"))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON)
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      attr.set(asset.Tag.SYNC_TYPE, asset.SyncType.THIS_DEVICE)
      attr.set(asset.Tag.REQUIRE_PASSWORD_SET, false)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let res = await queryAttrPromise(alias)
      expect(res.get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('addAllTags'))
      expect(res.get(asset.Tag.DATA_LABEL_NORMAL_1)).assertDeepEquals(stringToArray('normal_label1'))
      expect(res.get(asset.Tag.DATA_LABEL_NORMAL_2)).assertDeepEquals(stringToArray('normal_label2'))
      expect(res.get(asset.Tag.DATA_LABEL_NORMAL_3)).assertDeepEquals(stringToArray('normal_label3'))
      expect(res.get(asset.Tag.DATA_LABEL_NORMAL_4)).assertDeepEquals(stringToArray('normal_label4'))
      expect(res.get(asset.Tag.DATA_LABEL_CRITICAL_1)).assertDeepEquals(stringToArray('critical_label1'))
      expect(res.get(asset.Tag.DATA_LABEL_CRITICAL_2)).assertDeepEquals(stringToArray('critical_label2'))
      expect(res.get(asset.Tag.DATA_LABEL_CRITICAL_3)).assertDeepEquals(stringToArray('critical_label3'))
      expect(res.get(asset.Tag.DATA_LABEL_CRITICAL_4)).assertDeepEquals(stringToArray('critical_label4'))
      expect(res.get(asset.Tag.ACCESSIBILITY)).assertEqual(asset.Accessibility.DEVICE_POWERED_ON)
      expect(res.get(asset.Tag.AUTH_TYPE)).assertEqual(asset.AuthType.ANY)
      expect(res.get(asset.Tag.SYNC_TYPE)).assertEqual(asset.SyncType.THIS_DEVICE)
      expect(res.get(asset.Tag.IS_PERSISTENT)).assertFalse()
      expect(res.get(asset.Tag.REQUIRE_PASSWORD_SET)).assertFalse()
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('addRequiredTags', 0, async () => {
      let alias = 'addRequiredTags'
      expect(await addAssetPromise(alias)).assertTrue()

      let res = await queryContentPromise(alias)
      expect(res.get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray("addRequiredTags"))
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray("demo_pwd"))
      expect(res.get(asset.Tag.ACCESSIBILITY)).assertEqual(asset.Accessibility.DEVICE_FIRST_UNLOCKED)
      expect(res.get(asset.Tag.AUTH_TYPE)).assertEqual(asset.AuthType.NONE)
      expect(res.get(asset.Tag.SYNC_TYPE)).assertEqual(asset.SyncType.NEVER)
      expect(res.get(asset.Tag.IS_PERSISTENT)).assertFalse()
      expect(res.get(asset.Tag.REQUIRE_PASSWORD_SET)).assertFalse()
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('addSameAliasDefaultThrowError', 0, async () => {
      let alias = 'addSameAliasDefaultThrowError'
      expect(await addAssetPromise(alias)).assertTrue()

      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd_new"))
      expect(await addAssetPromiseResult(attr)).assertEqual(24000003)

      let res = await queryContentPromise(alias)
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray("demo_pwd"))
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('addSameAliasThrowError', 0, async () => {
      let alias = 'addSameAliasThrowError'
      expect(await addAssetPromise(alias)).assertTrue()

      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd_new"))
      attr.set(asset.Tag.CONFLICT_RESOLUTION, asset.ConflictResolution.THROW_ERROR)
      expect(await addAssetPromiseResult(attr)).assertEqual(24000003)

      let res = await queryContentPromise(alias);
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray("demo_pwd"))
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('addSameAliasOverwrite', 0, async () => {
      let alias = 'addSameAliasOverwrite'
      expect(await addAssetPromise(alias)).assertTrue()

      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd_new"))
      attr.set(asset.Tag.CONFLICT_RESOLUTION, asset.ConflictResolution.OVERWRITE)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let res = await queryContentPromise(alias);
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray("demo_pwd_new"))
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('addChineseData', 0, async () => {
      let alias = 'addChineseData'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      attr.set(asset.Tag.SECRET, stringToArray("杨玉树"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("杨玉树"))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let res = await queryContentPromise(alias)
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray("杨玉树"))
      expect(res.get(asset.Tag.DATA_LABEL_NORMAL_1)).assertDeepEquals(stringToArray("杨玉树"))
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('addMultipleSyncTypes', 0, async () => {
      let alias = 'addMultipleSyncTypes'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      attr.set(asset.Tag.SECRET, stringToArray(alias))
      attr.set(asset.Tag.SYNC_TYPE, asset.SyncType.THIS_DEVICE | asset.SyncType.TRUSTED_DEVICE)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let res = await queryAttrPromise(alias)
      expect(res.get(asset.Tag.SYNC_TYPE)).assertEqual(asset.SyncType.THIS_DEVICE | asset.SyncType.TRUSTED_DEVICE)
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('addEmptyAttr', 0, async () => {
      let attr: asset.AssetMap = new Map();
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })
  })

  describe('addAliasPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('addWithoutAlias', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.SECRET, stringToArray('addWithoutAlias'))
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addAliasWithMaxLen', 0, async () => {
      let attr: asset.AssetMap = new Map();
      let alias = new Uint8Array(256)
      alias.fill(97)
      attr.set(asset.Tag.ALIAS, alias)
      attr.set(asset.Tag.SECRET, stringToArray('addAliasWithMaxLen'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      expect(await removeAssetPromise(arrayToString(alias))).assertTrue()
    })

    it('addAliasWithMinLen', 0, async () => {
      let attr: asset.AssetMap = new Map();
      let alias = new Uint8Array(1)
      alias.fill(97)
      attr.set(asset.Tag.ALIAS, alias)
      attr.set(asset.Tag.SECRET, stringToArray('addAliasWithMinLen'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      expect(await removeAssetPromise(arrayToString(alias))).assertTrue()
    })

    it('addAliasInvalidLen', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, new Uint8Array(257));
      attr.set(asset.Tag.SECRET, stringToArray('addAliasInvalidLen'))
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.ALIAS, new Uint8Array(0));
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addAliasUnmatchedType', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, 0);
      attr.set(asset.Tag.SECRET, stringToArray('addAliasUnmatchedType'))
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.ALIAS, false);
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })
  })

  describe('addSecretPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('addWithoutSecret', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray('addWithoutSecret'))
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addSecretWithMaxLen', 0, async () => {
      let attr: asset.AssetMap = new Map();
      let secret = new Uint8Array(1024)
      secret.fill(97)
      attr.set(asset.Tag.SECRET, secret)
      attr.set(asset.Tag.ALIAS, stringToArray('addSecretWithMaxLen'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      expect(await removeAssetPromise('addSecretWithMaxLen')).assertTrue()
    })

    it('addSecretWithMinLen', 0, async () => {
      let attr: asset.AssetMap = new Map();
      let secret = new Uint8Array(1)
      secret.fill(97)
      attr.set(asset.Tag.SECRET, secret)
      attr.set(asset.Tag.ALIAS, stringToArray('addSecretWithMinLen'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      expect(await removeAssetPromise('addSecretWithMinLen')).assertTrue()
    })

    it('addSecretInvalidLen', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('addSecretInvalidLen'))
      attr.set(asset.Tag.SECRET, new Uint8Array(1025))
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.SECRET, new Uint8Array(0))
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addSecretUnmatchedType', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SECRET, 0);
      attr.set(asset.Tag.ALIAS, stringToArray('addSecretUnmatchedType'))
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.SECRET, false);
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })
  })

  describe('addInvalidArgumentPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('addAccessibilityInvalid', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SECRET, stringToArray('addAccessibilityInvalid'))
      attr.set(asset.Tag.ALIAS, stringToArray('addAccessibilityInvalid'))
      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON - 1)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED + 1)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addAuthTypeInvalid', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SECRET, stringToArray('addAuthTypeInvalid'))
      attr.set(asset.Tag.ALIAS, stringToArray('addAuthTypeInvalid'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE + 1)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY + 1)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addSyncTypeInvalid', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SECRET, stringToArray('addSyncTypeInvalid'))
      attr.set(asset.Tag.ALIAS, stringToArray('addSyncTypeInvalid'))
      attr.set(asset.Tag.SYNC_TYPE, asset.SyncType.NEVER - 1)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.SYNC_TYPE, (asset.SyncType.THIS_DEVICE | asset.SyncType.TRUSTED_DEVICE) + 1)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addSyncTypeMaxValue', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SECRET, stringToArray('addSyncTypeMaxValue'))
      attr.set(asset.Tag.ALIAS, stringToArray('addSyncTypeMaxValue'))
      attr.set(asset.Tag.SYNC_TYPE, (asset.SyncType.THIS_DEVICE | asset.SyncType.TRUSTED_DEVICE))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      expect(await removeAssetPromise('addSyncTypeMaxValue')).assertTrue()
    })

    it('addConflictResolutionInvalid', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SECRET, stringToArray('addConflictResolutionInvalid'))
      attr.set(asset.Tag.ALIAS, stringToArray('addConflictResolutionInvalid'))
      attr.set(asset.Tag.CONFLICT_RESOLUTION, asset.ConflictResolution.OVERWRITE - 1)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.CONFLICT_RESOLUTION, asset.ConflictResolution.THROW_ERROR + 1)
      expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addLabelMaxLen', 0, async () => {
      let label = new Uint8Array(512)
      label.fill(97)
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.SECRET, stringToArray('addLabelMaxLen'))
      attr.set(asset.Tag.ALIAS, stringToArray('addLabelMaxLen'))
      attr.set(asset.Tag.DATA_LABEL_CRITICAL_1, label)
      attr.set(asset.Tag.DATA_LABEL_CRITICAL_2, label)
      attr.set(asset.Tag.DATA_LABEL_CRITICAL_3, label)
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, label)
      attr.set(asset.Tag.DATA_LABEL_NORMAL_2, label)
      attr.set(asset.Tag.DATA_LABEL_NORMAL_3, label)
      attr.set(asset.Tag.DATA_LABEL_NORMAL_4, label)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      expect(await removeAssetPromise('addLabelMaxLen')).assertTrue()
    })

    it('addLabelInvalidLen', 0, async () => {
      let labels = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4
      ]
      for (let i = 0; i < labels.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(asset.Tag.SECRET, stringToArray('addLabelInvalidLen'))
        attr.set(asset.Tag.ALIAS, stringToArray('addLabelInvalidLen'))
        attr.set(labels[i], new Uint8Array(0))
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        attr.set(labels[i], new Uint8Array(513))
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('addBoolTagUnmatchedType', 0, async () => {
      let tags = [asset.Tag.REQUIRE_PASSWORD_SET, asset.Tag.IS_PERSISTENT]
      for (let i = 0; i < tags.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(asset.Tag.SECRET, stringToArray('addBoolTagUnmatchedType'))
        attr.set(asset.Tag.ALIAS, stringToArray('addBoolTagUnmatchedType'))
        attr.set(tags[i], 0)
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        attr.set(tags[i], new Uint8Array(1))
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('addBytesTagUnmatchedType', 0, async () => {
      let labels = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4
      ]
      for (let i = 0; i < labels.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(asset.Tag.SECRET, stringToArray('addLabelUnmatchedType'))
        attr.set(asset.Tag.ALIAS, stringToArray('addLabelUnmatchedType'))
        attr.set(labels[i], 0)
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        attr.set(labels[i], true)
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('addNumberTagWithUnmatchedType', 0, async () => {
      let labels = [
        asset.Tag.ACCESSIBILITY, asset.Tag.AUTH_TYPE, asset.Tag.SYNC_TYPE, asset.Tag.CONFLICT_RESOLUTION
      ]
      for (let i = 0; i < labels.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(asset.Tag.SECRET, stringToArray('addNumberTagWithUnmatchedType'))
        attr.set(asset.Tag.ALIAS, stringToArray('addNumberTagWithUnmatchedType'))
        attr.set(labels[i], new Uint8Array(1))
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        attr.set(labels[i], true)
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('addUnsupportedTag', 0, async () => {
      let tagBytes = [asset.Tag.AUTH_CHALLENGE, asset.Tag.AUTH_TOKEN]
      for (let i = 0; i < tagBytes.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(asset.Tag.SECRET, stringToArray('addUnsupportedTag'))
        attr.set(asset.Tag.ALIAS, stringToArray('addUnsupportedTag'))
        attr.set(tagBytes[i], new Uint8Array(1))
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let tagNumber = [asset.Tag.AUTH_VALIDITY_PERIOD, asset.Tag.RETURN_LIMIT, asset.Tag.RETURN_OFFSET, asset.Tag.RETURN_ORDERED_BY, asset.Tag.RETURN_TYPE]
      for (let i = 0; i < tagNumber.length; i++) {
        let attr: asset.AssetMap = new Map()
        attr.set(asset.Tag.SECRET, stringToArray('addUnsupportedTag'))
        attr.set(asset.Tag.ALIAS, stringToArray('addUnsupportedTag'))
        attr.set(tagNumber[i], 1)
        expect(await addAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })
  })
}